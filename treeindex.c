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
	uint64_t type;
	uint64_t num_children;
};
typedef struct node_v1 node_t;

struct node_v1_ser {
	unsigned char* name;
	uint64_t name_len;
	uint64_t type;
	uint64_t num_children;
};
typedef struct node_v1_ser node_ser_t;


serialize_node(node_ser_t *ser, node_t *node) {
	ser->name_len = node->name_len;
	ser->type = node->type;
	ser->num_children = node->num_children;
	ser->name = (unsigned char*)malloc(node->name_len);
	memcpy(ser->name, node->name, node->name_len);
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
	node_ser_t ser;

	serialize_node(&ser, node);
	fwrite(&ser, sizeof(node_ser_t), 1, file);
	for (i = 0; i < node->num_children; i++) {
		serialize_index(&(node->children[i]), file, counter);
	}
}

int main(void) {
	node_t index;
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

	print_index(
		&index,
		0
		);

	file = fopen("index.ti", "wb");
	serialize_index(&index, file, NULL);
	fclose(file);

	return(0);
}

