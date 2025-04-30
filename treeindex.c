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

typedef struct node_t node_t;
struct node_t {
	node_t* children;
	struct dirent dent;
	unsigned int num_children;
};

void action_print(struct dirent* dent, unsigned int depth) {
	printf("%.*s%s %i\n", depth, PAD, dent->d_name, dent->d_type);
}

void recurse(
		char* path,
		unsigned int depth,
		void (*action)(struct dirent*, unsigned int)
		) {
	DIR *dir;
	struct dirent *dent;

	dir = opendir(path);
	while ((dent = readdir(dir)) != NULL) {

		/*
		printf("%.*s%s %i\n", depth, PAD, dent->d_name, dent->d_type);
		*/
		action(dent, depth);

		if (dent->d_type == TYPE_DIR) {
			if (strcmp(".", dent->d_name) == 0) {
				continue;
			}
			if (strcmp("..", dent->d_name) == 0) {
				continue;
			}
			sprintf(next_path, "%s/%s", path, dent->d_name);
			recurse(next_path, depth + 1, action);
		}
	}

	closedir(dir);
}

node_t* append_node(node_t *node) {
	node_t *new_child;

	node->num_children++;
	node->children = (node_t*)
		realloc(node->children, sizeof(node_t) * node->num_children);
	/*
	TODO what is actually wrong with this??
	new_child = (
		node->children
		+ (sizeof(node_t) * (node->num_children - 1))
		);
		*/
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

void init_node(node_t *node) {
	node->num_children = 0;
	node->children = NULL;
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
		init_node(new_node);
		memcpy(&(new_node->dent), dent, sizeof(struct dirent));

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
			"%.*s%s %i\n",
			depth,
			PAD,
			child->dent.d_name,
			child->dent.d_type
			);

		if (child->dent.d_type == TYPE_DIR) {
			print_index(child, depth + 1);
		}
	}
}

void serialize_index(
		node_t *node,
		unsigned int *counter,
		) {
}

int main(void) {
	node_t index;
	init_node(&index);

	recurse_index(
		"/home/maybetree/proj/etec/EE3/L11/litstudy",
		0,
		&index
		);

	print_index(
		&index,
		0
		);

	return(0);
}

