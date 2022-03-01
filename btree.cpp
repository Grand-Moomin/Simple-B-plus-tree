#include <iostream>
using namespace std;

template<typename T>
struct Node{
    bool is_leaf;
    T *keys;
    int curr_key_num;
    union{
        Node<T> **children;
        void **leaf_contents;
    };
    Node<T> *parent;
    Node<T> *prev_sibling, *next_sibling;

    Node(int key_num_per_node);
    ~Node();

    void print_keys();
};

template<typename T>
void Node<T>::print_keys()
{
    for(int i = 0; i < curr_key_num; ++i){
        if(!is_leaf)
            cout<<children[i]<<' ';
        cout<<keys[i]<<' ';
    }
    if(!is_leaf)
        cout<<children[curr_key_num];
    cout<<endl;
}

template<typename T>
Node<T>::Node(int key_num_per_node)
{
    this->parent = this->next_sibling = this->prev_sibling = nullptr;
    this->curr_key_num = 0;
    keys = new T [key_num_per_node];
    fill(keys, keys+key_num_per_node, 0);
    children = new Node<T>* [key_num_per_node + 1];
    fill((char*)children, (char *)(children + key_num_per_node + 1), 0);
}

template<typename T>
Node<T>::~Node()
{
    delete [] keys;
    delete [] children;
}

template<typename T>
class BTree{
protected:
    int key_num_per_node;
    Node<T> *root;
    bool insert_to_parent(T k, Node<T> *parent, Node<T> *new_leaf);
    void lazy_delete_from_parent(T k, Node<T> *parent, Node<T> *child);
    T find_max_element_from_descendents(Node<T> *cursor); //We don't merge or redistribute nodes that become less half full.

public:
    BTree(int key_num_per_node);
    inline Node<T>* get_root_node(){return root;}
    Node<T>* search_key(T k);
    bool insert(T k);
    bool lazy_delete(T k); //We don't merge or redistribute nodes that become less half full.

    void print_tree(Node<T> *cursor, int level = 1);
};

template<typename T>
void BTree<T>::print_tree(Node<T> *cursor, int level)
{
    if(!cursor) 
        return;

    cout<<level<<':'<<cursor<<':';
    cursor->print_keys();
    if(cursor->prev_sibling) cout<<"Prev sibling: "<<cursor->prev_sibling<<endl;
    if(cursor->next_sibling) cout<<"Next sibling: "<<cursor->next_sibling<<endl;
    if(cursor->parent) cout<<"Parent: "<<cursor->parent<<endl;

    for(int i=0; i<cursor->curr_key_num + 1; ++i){
        print_tree(cursor->children[i], level + 1);
    }
}

template<typename T>
BTree<T>::BTree(int key_num_per_node){
    if(key_num_per_node < 3)
        key_num_per_node = 3;
    root = nullptr;
    this->key_num_per_node = key_num_per_node;
}

template<typename T>
Node<T>* BTree<T>::search_key(T k)
{
    int i;
    Node<T> *cursor = root;

    if(!root) return root;
    while(cursor->is_leaf == false){
        for(i = 0; i < cursor->curr_key_num; ++i){
            if(k <= cursor->keys[i])
                break;
        }
        cursor = cursor->children[i];
    }
    for(i = 0; i < cursor->curr_key_num; ++i){
        if(k == cursor->keys[i]){
            return cursor;
        }
    }
    return nullptr;
}

template<typename T>
bool BTree<T>::insert(T k)
{
    int i, j;
    Node<T> *cursor = root;

    if(!root){
        root = new Node<T>(key_num_per_node);
        root->is_leaf = true;
        root->keys[0] = k;
        root->curr_key_num = 1;
        return true;
    }
    else{
        //Scurry to leaf node.
        while(cursor->is_leaf == false){
            for(i = 0; i < cursor->curr_key_num; ++i){
                if(k <= cursor->keys[i])
                    break;
            }
            cursor = cursor->children[i];
        }
        //Find the position to insert new key in the leaf node.
        for(i = 0; i < cursor->curr_key_num; ++i){
            if(k < cursor->keys[i])
                break;
            else if(k == cursor->keys[i])
                return false; //Duplicated key not permitted.
        }
        //If there is still some vacancy in the leaf node, insert it.
        if(cursor->curr_key_num < key_num_per_node){
            for(j = cursor->curr_key_num; j > i; --j){
                cursor->keys[j] = cursor->keys[j-1];
            }
            cursor->keys[i] = k;
            cursor->curr_key_num++;
            return true;
        }
        //Unfortunately, there is no vacancy in the leaf node, we have to split the node.
        else{
            T temp_keys[key_num_per_node + 1];
            Node<T> *new_leaf = new Node<T>(key_num_per_node);
            for(j = 0; j < key_num_per_node + 1; ++j){
                if(j < i)
                    temp_keys[j] = cursor->keys[j];
                else if(j == i)
                    temp_keys[j] = k;
                else
                    temp_keys[j] = cursor->keys[j - 1];
            }
            cursor->curr_key_num = (key_num_per_node + 1)/2;
            new_leaf->curr_key_num = (key_num_per_node + 1) - cursor->curr_key_num;
            new_leaf->is_leaf = true;
            //For the time being, we set the parent of the new leaf node identical with the original node's.
            //Its parent might change if there is no vacancy in the parent node.
            new_leaf->parent = cursor->parent;
            for(i = 0; i < cursor->curr_key_num; ++i){
                cursor->keys[i] = temp_keys[i];
            }
            for(i = 0, j = cursor->curr_key_num; i < new_leaf->curr_key_num; ++i, ++j){
                new_leaf->keys[i] = temp_keys[j];
            }
            //Adjust siblings of cursor and new leaf.
            new_leaf->prev_sibling = cursor;
            if(cursor->next_sibling)
                cursor->next_sibling->prev_sibling = new_leaf;
            new_leaf->next_sibling = cursor->next_sibling;
            cursor->next_sibling = new_leaf;
            //If cursor's parent points to the root node, create a brand new root.
            if(!cursor->parent){
                Node<T> *new_root = new Node<T>(key_num_per_node);
                new_root->curr_key_num = 1;
                new_root->is_leaf = false;
                new_root->parent = nullptr;
                new_root->keys[0] = cursor->keys[cursor->curr_key_num - 1];
                new_root->children[0] = cursor;
                new_root->children[1] = new_leaf;
                root = new_root;
                cursor->parent = new_leaf->parent = new_root;
                return true;
            }
            //Or else, insert the last key of split cursor to its parent. The parent of new leaf will be changed if the parent node is full.
            else{
                return insert_to_parent(cursor->keys[cursor->curr_key_num - 1], cursor->parent, new_leaf);
            }
        }
    }
}

template <typename T>
bool BTree<T>::insert_to_parent(T k, Node<T> *parent, Node<T> *new_leaf)
{
    int i, j;

    //Find the position where the key k should be inserted.
    for(i = 0; i < parent->curr_key_num; ++i){
        if(k < parent->keys[i])
            break;
    }
    //If there is some vacancy in the parent node, insert it directly.
    if(parent->curr_key_num < key_num_per_node){
        for(j = parent->curr_key_num; j > i; --j){
            parent->keys[j] = parent->keys[j - 1];
            parent->children[j + 1] = parent->children[j];
        }
        parent->keys[i] = k;
        parent->children[i + 1] = new_leaf;
        parent->curr_key_num++;
        return true;
    }
    //Unlucky. We have split the parent as it is stuffed with keys.
    else{
        T temp_keys[key_num_per_node + 1];
        Node<T> *temp_children[key_num_per_node + 2];
        Node<T> *new_parent = new Node<T>(key_num_per_node);
        for(j = 0; j < key_num_per_node + 1; ++j){
            if(j < i){
                temp_keys[j] = parent->keys[j];
                temp_children[j] = parent->children[j];
            }
            else if(j == i){
                temp_keys[j] = k;
                temp_children[j] = parent->children[j];
                temp_children[j + 1] = new_leaf;
            }
            else{
                temp_keys[j] = parent->keys[j - 1];
                temp_children[j + 1] = parent->children[j];
            }
        }
        parent->curr_key_num = (key_num_per_node + 1)/2 + 1;
        new_parent->curr_key_num = key_num_per_node + 1 - parent->curr_key_num;
        new_parent->is_leaf = false;
        new_parent->parent = parent->parent;

        for(i = 0; i < parent->curr_key_num; ++i){
            parent->keys[i] = temp_keys[i];
            parent->children[i] = temp_children[i];
        }
        parent->children[i-1]->next_sibling = nullptr;//Sever sibling connection since parents changed.
        for(j = 0, i = parent->curr_key_num; j < new_parent->curr_key_num + 1; ++j, ++i){
            if(j < new_parent->curr_key_num)
                new_parent->keys[j] = temp_keys[i];
            new_parent->children[j] = temp_children[i];
            //Change the parent of the children to the new parent.
            if(new_parent->children[j])
                new_parent->children[j]->parent = new_parent;
        }
        new_parent->children[0]->prev_sibling = nullptr;//Sever sibling connection since parents changed.
        //Adjust siblings.
        new_parent->prev_sibling = parent;
        if(parent->next_sibling)
            parent->next_sibling->prev_sibling = new_parent;
        new_parent->next_sibling = parent->next_sibling;
        parent->next_sibling = new_parent;
        //In case that the root node is the grandparent.
        if(!parent->parent){
            Node<T> *new_root = new Node<T>(key_num_per_node);
            new_root->curr_key_num = 1;
            new_root->is_leaf = false;
            new_root->parent = nullptr;
            new_root->keys[0] = parent->keys[parent->curr_key_num - 1];
            new_root->children[0] = parent;
            new_root->children[1] = new_parent;
            root = new_root;
            parent->parent = new_parent->parent = new_root;
        }
        //Or, insert the last key of current parent to its parent. 
        else{
            insert_to_parent(parent->keys[parent->curr_key_num - 1], parent->parent, new_parent);
        }
        //Last key of current parent is meaningless, so we ignore it by decreasing the key number by 1.
        parent->curr_key_num--;
        return true;
    }
}

template<typename T>
bool BTree<T>::lazy_delete(T k)
{
    int i, j;
    Node<T> *cursor = root;
    if(!root)
        return false;
    while(cursor->is_leaf == false){
        for(i = 0; i < cursor->curr_key_num; ++i){
            if(k <= cursor->keys[i])
                break;
        }
        cursor = cursor->children[i];
    }
    for(i = 0; i < cursor->curr_key_num; ++i){
        if(k == cursor->keys[i])
            break;
    }
    //Leaf node not found.
    if(i >= cursor->curr_key_num)
        return false;
    //If the current leaf is the root node
    if(!cursor->parent){
        if(cursor->curr_key_num > 1){
            for(j = i; j < cursor->curr_key_num - 1; ++j){
                cursor->keys[j] = cursor->keys[j + 1];
            }
            cursor->curr_key_num--;
        }
        else{
            delete cursor;
            root = nullptr;
        }
        return true;
    }
    //Current leaf is not root node
    else{
        for(j = i; j < cursor->curr_key_num - 1; ++j){
            cursor->keys[j] = cursor->keys[j + 1];
        }
        cursor->curr_key_num--;
        //Remove k from parent, grandparent, etc.
        lazy_delete_from_parent(k, cursor->parent, cursor);
        //If current cursor has no keys, delete it.
        if(!cursor->curr_key_num){
            //Adjust sibling relationship.
            if(cursor->next_sibling)
                cursor->next_sibling->prev_sibling = cursor->prev_sibling;
            if(cursor->prev_sibling){
                cursor->prev_sibling->next_sibling = cursor->next_sibling;
            }
            //Finally, the current cursor can be safely deleted.
            delete cursor;
        }
        return true;
    }
}

template<typename T>
void BTree<T>::lazy_delete_from_parent(T k, Node<T> *parent, Node<T> *child)
{
    int i, j;
    if(!parent)
        return;
    //If the child does not have any key, delete it from its parent.
    if(!child->curr_key_num){
        for(i = 0; i < parent->curr_key_num; ++i){
            if(parent->children[i] == child){
                break;
            }
        }
        for(j = i; j < parent->curr_key_num - 1; ++j){
            parent->keys[j] = parent->keys[j + 1];
            parent->children[j] = parent->children[j + 1];
        }
        parent->children[j] = parent->children[j + 1];
        parent->children[j + 1] = nullptr;
        parent->curr_key_num--;
        
        //Corner case: Parent does not have any key now, but its first child still exists.
        if(!parent->curr_key_num){
            if(parent->children[0] && parent->children[0]->curr_key_num > 0){
                //If parent is root, change the root to its first and only child before delete it.
                if(parent == root){
                    root = parent->children[0];
                    root->parent = nullptr;
                    delete parent;
                    return;
                }
                //If the parent is not the root, change the first key of parent to its only child's descendents largest key.
                else{
                    parent->keys[0] = find_max_element_from_descendents(parent->children[0]);
                    parent->children[1] = nullptr;
                    parent->curr_key_num++;
                    //If we reach here, the parent is rather a peculiar one which has only one child "children[0]".
                }
            }
        }

        lazy_delete_from_parent(k, parent->parent, parent);

        //If parent is empty after deletion, it should be obliterated as well.
        if(!parent->curr_key_num){
            //Adjust sibling relationship.
            if(parent->next_sibling)
                parent->next_sibling->prev_sibling = parent->prev_sibling;
            if(parent->prev_sibling)
                parent->prev_sibling->next_sibling = parent->next_sibling;
            //Finally, the parent can be safely deleted.
            delete parent;
        }
    }

    //If the child still has some keys, then it should be kept.
    else{
        for(i = 0; i < parent->curr_key_num; ++i){
            if(parent->keys[i] == k){
                break;
            }
        }
        //k not found in parent keys.
        if(i >= parent->curr_key_num){
            lazy_delete_from_parent(k, parent->parent, parent);
        }
        //k found in parent keys.
        else{
            //We exclude the case that the child has no keys in the beginning, so we can safely borrow the largest leaf key from all its descendents.
            parent->keys[i] = find_max_element_from_descendents(child);
            lazy_delete_from_parent(k, parent->parent, parent);
        }
    }
}

template<typename T>
T BTree<T>::find_max_element_from_descendents(Node<T> *cursor){
    while(cursor->is_leaf == false){
        cursor = (cursor->children[cursor->curr_key_num])? \
                        cursor->children[cursor->curr_key_num] : \
                        cursor->children[cursor->curr_key_num - 1];
    }
    return cursor->keys[cursor->curr_key_num - 1];
}

void simple_tree_test()
{
    BTree<int> btree(3);
    int arr[] = {4, 7, 10, 9 ,5, 3, 20, 33, 56, 79, 2, 84, 80, 76, 65, 101, 147, 120, \
                95, 136, 124, 122, 121, 128, 134, 133, 145, 149, 150, 200, 168, 186, \
                170, 34, 44, 46, 48, 171, 174, 176, 178, 111, 118, 119, 113, 114, 115,\
                160, 250, 210, 202, 203, 405, 304, 399, 220, 207, 208, 209, 211, 212,\
                231, 232, 233, 223};
    int n = sizeof(arr)/sizeof(arr[0]);
    cout<<n<<endl;
    for(int i = 0; i < n; ++i){
        btree.insert(arr[i]);
    }
    //btree.print_tree(btree.get_root_node());
    for(int i = 0; i < 58; ++i){
        btree.lazy_delete(arr[i]);
    }
    for(int i = 0; i < 25; ++i){
        btree.insert(arr[i]);
    }
    for(int i = 0; i < 60; ++i){
        btree.lazy_delete(arr[i]);
    }
    btree.print_tree(btree.get_root_node());
}

int main()
{
    simple_tree_test();
}