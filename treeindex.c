#include <dirent.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdint.h> 

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
	fread(&(node->type), sizeof(node->type), 1, file);
	fread(&(node->num_children), sizeof(node->num_children), 1, file);

	node->name = (unsigned char*)malloc(node->name_len);
	fread(node->name, 1, node->name_len, file);

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

	dir = opendir(path);
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


int main(void) {
	node_t index;
	node_t index_deser;
	FILE *file;

	index.num_children = 0;
	index.children = NULL;
	index.name = (unsigned char*)"/ayy";
	index.name_len = 4;
	index.type = TYPE_DIR;

	recurse_index(
		"/home/maybetree/proj/etec/EE3/L11/litstudy",
		0,
		&index
		);


	file = fopen("index.ti", "wb");
	serialize_index(&index, file, NULL);
	fclose(file);

	/*
	print_index(
		&index,
		0
		);
	*/

	file = fopen("index.ti", "rb");
	deserialize_index(&index_deser, file, NULL);
	fclose(file);

	print_index(
		&index_deser,
		0
		);

	return(0);
}

