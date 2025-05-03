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
};
typedef struct node_v1 node_t;

serialize_node(node_t *node, FILE *file) {
	fwrite(&(node->name_len), sizeof(node->name_len), 1, file);
	fwrite(&(node->type), sizeof(node->type), 1, file);
	fwrite(&(node->num_children), sizeof(node->num_children), 1, file);

	fwrite(node->name, node->name_len, 1, file);
}

deserialize_node(node_t *node, FILE *file) {
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

void init_node(node_t *node, struct dirent *dent) {
	node->num_children = 0;
	node->children = NULL;

	node->name_len = strlen(dent->d_name);
	node->name = (unsigned char*)malloc(node->name_len);
	memcpy(node->name, dent->d_name, node->name_len);

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
	
	dir = opendir(path); check_errno();
	while ((dent = readdir(dir)) != NULL) {

		new_node = append_node(node);
		init_node(new_node, dent);

		if (dent->d_type == TYPE_DIR) {
			if (strcmp(".", dent->d_name) == 0) {
				continue;
			}
			if (strcmp("..", dent->d_name) == 0) {
				continue;
			}
			sprintf(next_path, "%s/%s", path, dent->d_name);
			/*fprintf(stderr, "%s\n", next_path);*/
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
			child->name_len,
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
			child->name_len,
			child->name,
			child->type
			);
	}
}

void serialize_index(
		node_t *node,
		FILE *file,
		unsigned int *counter
		) {
	int i;

	serialize_node(node, file);
	for (i = 0; i < node->num_children; i++) {
		serialize_index(&(node->children[i]), file, counter);
	}
}

void deserialize_index(
		node_t *node,
		FILE *file,
		unsigned int *counter
		) {
	int i;

	deserialize_node(node, file);

	for (i = 0; i < node->num_children; i++) {
		deserialize_index(&(node->children[i]), file, counter);
	}
}

void deserialize_index_progressive(
		node_t *node,
		FILE *file,
		char *path
		) {
	char* next_path;
	int path_seg_len;
	int i;

	next_path = strchr(path, '/');
	path_seg_len = next_path - path;

	deserialize_node(node, file);

	if (path_seg_len != node->name_len) {
		return;
	}

	if (strncmp((char*)(node->name), path, path_seg_len) != 0) {
		return;
	}

	/*
	 * TODO this wont work need to store an internal reference
	 * i.e. how many children to skip
	 */

	for (i = 0; i < node->num_children; i++) {
		deserialize_index_progressive(&(node->children[i]), file, next_path);
	}
}

node_t* pad_index(node_t *node, char *path) {
	node_t *child;
	char* next_path;
	int path_seg_len;

	if (path[0] == '/') {
		path++;
	}

	next_path = strchr(path, '/');
	path_seg_len = next_path - path;

	node->type = 4;
	node->name_len = path_seg_len;
	node->name = (unsigned char*)malloc(path_seg_len);
	strncpy((char*)node->name, path, path_seg_len);

	if (next_path == NULL) {
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
		fprintf(stderr, "%s\n", token);
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
	node_t index;
	node_t index_deser;
	node_t *index_start;
	FILE *file;

	if (argc < 4) {
		puts("err");
		return 1;
	}

	index.num_children = 0;
	index.children = NULL;
	index.name = (unsigned char*)"/ayy";
	index.name_len = 4;
	index.type = TYPE_DIR;

	if (strcmp("scan", argv[2]) == 0) {
		index_start = pad_index(&index, argv[3]);
		recurse_index(
			argv[3],
			0,
			index_start
			);
		file = fopen(argv[1], "wb"); check_errno();
		serialize_index(&index, file, NULL);
		fclose(file);
	}
	else if (strcmp("ls", argv[2]) == 0) {
		file = fopen(argv[1], "rb"); check_errno();
		deserialize_index(&index, file, NULL);
		fclose(file);

		ls_index(
			&index,
			argv[3]
			);
	}
	else {
		puts("unknown");
		return 1;
	}


	return 0;
}

