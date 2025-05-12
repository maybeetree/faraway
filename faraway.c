#include <dirent.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdint.h> 
#include <argp.h>
#include <errno.h>

const unsigned int TYPE_DIR = 4;
char next_path[2048];
char PAD[509]= \
	"                                                                    "\
	"                                                                    "\
	"                                                                    "\
	"                                                                    "\
	"                                                                    "\
	"                                                                    "\
	"                                                                    "\
	"                                 ";

void check_errno() {
	if (errno != 0) {
		fprintf(stderr, "System error: %i\n", errno);
		exit(errno);
	}
}

struct node_v1 {
	struct node_v1* children;
	unsigned char* name;
	uint64_t name_len;
	unsigned char type;
	uint64_t num_children;
	uint64_t *child_offsets;
};
typedef struct node_v1 node_t;

serialize_node(node_t *node, FILE *file) {
	fwrite(&(node->name_len), sizeof(node->name_len), 1, file);
	fwrite(&(node->type), sizeof(node->type), 1, file);
	fwrite(&(node->num_children), sizeof(node->num_children), 1, file);

	fwrite(node->name, node->name_len, 1, file);
}

void deserialize_node(node_t *node, FILE *file) {
	fread(&(node->name_len), sizeof(node->name_len), 1, file);
	check_errno();
	fread(&(node->type), sizeof(node->type), 1, file);
	check_errno();
	fread(&(node->num_children), sizeof(node->num_children), 1, file);
	check_errno();

	node->name = (unsigned char*)malloc(node->name_len);
	fread(node->name, 1, node->name_len, file);
	check_errno();

	node->children = (node_t*)
		malloc(node->num_children * sizeof(node_t));

	node->child_offsets = (uint64_t*)
		malloc(sizeof(uint64_t) * node->num_children);
	fread(node->child_offsets, sizeof(uint64_t), node->num_children, file);
}

node_t* append_node(node_t *node) {
	node_t *new_child;

	node->num_children++;
	node->children = (node_t*)
		realloc(node->children, sizeof(node_t) * node->num_children);
	new_child = &(node->children[node->num_children - 1]);

	/*
	fprintf(
		stderr,
		"Grow node %p to size %i array %p child %p \n",
		node, node->num_children, node->children, new_child
		);
	*/

	return new_child;
}

node_t* get_child_by_name(node_t *node, char *name) {
	node_t *child;
	int i;

	for (i = 0; i < node->num_children; i++) {
		child = &(node->children[i]);
		if (
			strlen(name) == child->name_len
			&&
			strcmp(name, (char*)child->name) == 0
			) {
			
			return child;
		}
	}
	return NULL;
}

void init_node(
		node_t *node,
		struct dirent *dent,
		char* alt_name
		) {
	
	if (alt_name == NULL) {
		alt_name = dent->d_name;
	}

	node->num_children = 0;
	node->children = NULL;

	node->name_len = strlen(alt_name);
	node->name = (unsigned char*)malloc(node->name_len);
	memcpy(node->name, alt_name, node->name_len);

	node->type = dent->d_type;
}

void recurse_index(
		char *path,
		unsigned int depth,
		node_t *node
		) {
	DIR *dir;
	struct dirent *dent;
	node_t *new_node;
	char next_path[1000];
	char* this_dir_name;
	char* this_dir_name_buf;
	char* path2;

	/*
	path2 = (char*)malloc(strlen(path) + 1);
	strcpy(path2, path);

	this_dir_name = strtok(path2, "/");
	while (this_dir_name != NULL) {
		this_dir_name_buf = this_dir_name;
		this_dir_name = strtok(NULL, "/");
	}
	*/
	
	dir = opendir(path); check_errno();
	while ((dent = readdir(dir)) != NULL) {

		check_errno();

		new_node = append_node(node);
		init_node(new_node, dent, NULL);

		if (dent->d_type == TYPE_DIR) {
			if (strcmp(".", dent->d_name) == 0) {
				/*
				init_node(node, dent, this_dir_name_buf);
				*/
				continue;
			}
			if (strcmp("..", dent->d_name) == 0) {
				continue;
			}

			sprintf(next_path, "%s/%s", path, dent->d_name);
			/*
			fprintf(stderr, "%s %s %s\n", path, dent->d_name, next_path);
			*/

			recurse_index(next_path, depth + 1, new_node);
		}
	}

	closedir(dir);
}

void print_index(node_t *node, unsigned int depth) {
	unsigned int i;
	node_t *child;

	for (i = 0; i < node->num_children; i++) {
		child = &(node->children[i]);

		printf(
			"%.*s%.*s %i\n",
			depth,
			PAD,
			(int)(child->name_len),
			child->name,
			child->type
			);

		print_index(child, depth + 1);
	}
}

void print_children(node_t *node) {
	unsigned int i;
	node_t *child;

	for (i = 0; i < node->num_children; i++) {
		child = &(node->children[i]);

		printf(
			"%.*s %i\n",
			(int)(child->name_len),
			child->name,
			child->type
			);
	}
}

uint64_t serialize_index(
		node_t *node,
		FILE *file
		) {
	int i;
	uint64_t self_position;
	uint64_t self_position_end;
	uint64_t position_children_positions;
	uint64_t* children_positions;

	children_positions = (uint64_t*)malloc(node->num_children * sizeof(uint64_t));

	self_position = ftell(file);
	serialize_node(node, file);
	position_children_positions = ftell(file);
	check_errno();

	/* TODO write zeros instead? */
	fwrite(children_positions, node->num_children * sizeof(uint64_t), 1, file);
	check_errno();

	for (i = 0; i < node->num_children; i++) {
		children_positions[i] = serialize_index(&(node->children[i]), file);
	}

	self_position_end = ftell(file);
	check_errno();
	fseek(file, position_children_positions, SEEK_SET);
	check_errno();
	fwrite(children_positions, node->num_children * sizeof(uint64_t), 1, file);
	check_errno();
	fseek(file, self_position_end, SEEK_SET);
	check_errno();

	free(children_positions);

	return self_position;
}

void deserialize_index(
		node_t *node,
		FILE *file
		) {
	int i;

	deserialize_node(node, file);

	for (i = 0; i < node->num_children; i++) {
		deserialize_index(&(node->children[i]), file);
	}
}

void deserialize_index_progressive(
		node_t *node,
		FILE *file,
		char *path,
		int first
		) {
	int i;
	int pos_bak;
	node_t *child;
	char *dirname;

	while (path != NULL && path[0] == '/') {
		path++;
	}

	if (first) {
		deserialize_node(node, file);
	}

	dirname = strtok(path, "/");

	for (i = 0; i < node->num_children; i++) {
		child = &(node->children[i]);
		/*
		fprintf(stderr, "%i \n", node->child_offsets[i]);
		*/
		fseek(file, node->child_offsets[i], SEEK_SET);
		deserialize_node(child, file);

		/*
		fprintf(stderr, "%s %.*s %.*s\n", dirname, child->name_len, child->name, node->name_len, node->name);
		*/

		if (dirname == NULL) {
			/* Load all children of last node */
			continue;
		}

		if (
			strlen(dirname) == child->name_len
			&&
			strcmp(dirname, (char*)child->name) == 0
			) {

			
			deserialize_index_progressive(
					child, file, NULL, 0);

			return;
		}
	}

	if (dirname == NULL) {
		print_children(node);
		return;
	}

	fprintf(stderr, "Not found.\n");
	exit(69);
}

node_t* pad_index(node_t *node, char *path) {
	node_t *child;
	char* next_path;
	int path_seg_len;

	while (path[0] == '/') {
		path++;
	}

	next_path = strchr(path, '/');
	if (next_path == NULL) {
		path_seg_len = strlen(path);
	}
	else {
		path_seg_len = next_path - path;
	}

	node->type = 4;
	node->name_len = path_seg_len;
	node->name = (unsigned char*)malloc(path_seg_len);
	strncpy((char*)node->name, path, path_seg_len);

	if (next_path == NULL) {
		node->num_children = 0;
		node->children = NULL;
		return node;
	}

	child = append_node(node);

	return pad_index(child, next_path);
}

void ls_index(node_t *node, char *path) {
	char *token;
	node_t *this_node;

	token = strtok(path, "/");
	this_node = node;

	while (token) {
		/*
		fprintf(stderr, "%s\n", token);
		*/
		this_node = get_child_by_name(this_node, token);
		if (this_node == NULL) {
			fprintf(stderr, "Not found.\n");
			exit(69);
		}
		token = strtok(NULL, "/");
	}
	print_children(this_node);
}


int main(int argc, char **argv) {
	node_t root;
	node_t *index;
	node_t *index_start;
	FILE *file;

	if (argc < 4) {
		puts("err");
		return 1;
	}

	root.num_children = 0;
	root.children = NULL;
	root.name = (unsigned char*)"ROOT";
	root.name_len = 4;
	root.type = TYPE_DIR;

	index = append_node(&root);

	if (strcmp("scan", argv[2]) == 0) {

		/* remove trailing slashes */
		while (argv[3][strlen(argv[3])-1] == '/') {
			argv[3][strlen(argv[3])-1] = '\0';
		}

		index_start = pad_index(index, argv[3]);
		recurse_index(
			argv[3],
			0,
			index_start
			);
		file = fopen(argv[1], "wb"); check_errno();
		serialize_index(&root, file);
		fclose(file);
	}
	else if (strcmp("ls", argv[2]) == 0) {
		file = fopen(argv[1], "rb"); check_errno();
		/*deserialize_index(index, file);*/
		deserialize_index_progressive(&root, file, argv[3], 1);
		fclose(file);

		/*
		ls_index(
			&root,
			argv[3]
			);
			*/
	}
	else {
		puts("unknown");
		return 1;
	}


	return 0;
}

