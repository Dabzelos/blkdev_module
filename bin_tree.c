#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include "bin_tree.h"

node *create_node(uint64_bdd l, uint64_bdd p)
{
    node *root = kzalloc(1, sizeof(node));
    root->addrlog = l;
    root->addrphys = p;
    root->left = NULL;
    root->right = NULL;

    return root;
}
node *insert(node *curr, node *new)
{
    if (curr == NULL)
    {
        pr_info("1\n");
        return create_node(new->addrlog, new->addrphys);
    }
    if (new->addrlog > curr->addrlog)
    {
        pr_info("2\n");
        curr->right = insert(curr->right, new);
    }
    else if (new->addrlog < curr->addrlog)
    {
        pr_info("3\n");
        curr->left = insert(curr->left, new);
    }
    else
    {
        pr_info("4\n");
        pr_info("you trying to rewrite %lu \n", curr->addrlog);
        curr->addrlog = new->addrlog;
        curr->addrphys = new->addrphys;
        return curr;
    }
    pr_info("5\n");
    return curr;
}

void insert_b(node *curr, node *new)
{
    while (curr){

        if (curr->addrlog > new->addrlog){
            curr->right = insert(curr->right, new);
        }
        else if (curr->addrlog < new->addrlog){
            curr->left = insert(curr->left, new);
        }
        else{
            pr_info("you trying to rewrite %lu \n", curr->addrlog);
            curr->addrlog = new->addrlog;
            curr->addrphys = new->addrphys;
            break;
        }
    }
    curr = create_node(new->addrlog, new->addrphys);
}