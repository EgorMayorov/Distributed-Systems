#include <iostream>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <mpi.h>

#define TREE_ROOT 0

enum message_type : int8_t {REQUEST, MARKER, NOTHING};
enum pointer : int8_t {PARENT, LEFT, RIGHT};

struct tree {
    std::queue<int> request_queue;
    int rank;
    int root;
    int left;
    int right;
    pointer marker_pointer;
    bool marker;

    tree () {}

    // tree(int _rank, int _root, int _left, int _right, int _marker_pointer, bool _marker): 
    //     rank(_rank), root(_root), left(_left), right(_right), marker_pointer(_marker_pointer), marker(_marker) {}

    void critical () {
        if (access("critical.txt", F_OK) != -1) {
            std::cerr << "File exist" << std::endl;
            MPI_Finalize();
            exit(1);
        } else {
            fopen("critical.txt", "w");
            std::cout << "CRITICAL SECTION - rank " << rank << std::endl;
            sleep(rand() % 10);
            remove("critical.txt");
        }
    }

    void request (message_type message, int reciever) {
        if (message == MARKER) {
            if (!request_queue.empty()) {
                int requester = request_queue.front();
                request_queue.pop();
                std::cout << "send marker from " << rank << " to " << requester << std::endl;
                if (requester == rank) {
                    marker = true;
                    critical();
                } else if (requester == root) {
                    marker = false;
                    marker_pointer = PARENT;
                    message_type tmp = MARKER;
                    MPI_Send(&tmp, 1, MPI_INT8_T, requester, 0, MPI_COMM_WORLD);
                } else if (requester == left) {
                    marker = false;
                    marker_pointer = LEFT;
                    message_type tmp = MARKER;
                    MPI_Send(&tmp, 1, MPI_INT8_T, requester, 0, MPI_COMM_WORLD);
                } else if (requester == right) {
                    marker = false;
                    marker_pointer = RIGHT;
                    message_type tmp = MARKER;
                    MPI_Send(&tmp, 1, MPI_INT8_T, requester, 0, MPI_COMM_WORLD);
                } else {
                    std::cerr << "Invalid rank in queue!" << std::endl;
                    MPI_Finalize();
                    exit(1);
                }
                if (!request_queue.empty()) {
                    int receiver = request_queue.front();
                    request_queue.pop();
                    request(REQUEST, receiver);
                }
            }
        } else if (message == REQUEST) {
            std::cout << "request marker from " << reciever << " to " << rank << std::endl;
            request_queue.push(reciever);
            if (marker) {
                request(MARKER, rank);
            } else {
                if (marker_pointer == PARENT) {
                    message_type tmp = REQUEST;
                    if (rank != 0)
                        MPI_Send(&tmp, 1, MPI_INT8_T, root, 0, MPI_COMM_WORLD);
                } else if (marker_pointer == LEFT) {
                    message_type tmp = REQUEST;
                    MPI_Send(&tmp, 1, MPI_INT8_T, left, 0, MPI_COMM_WORLD);
                } else if (marker_pointer == RIGHT) {
                    message_type tmp = REQUEST;
                    MPI_Send(&tmp, 1, MPI_INT8_T, right, 0, MPI_COMM_WORLD);
                } else {
                    std::cerr << "Invalid marker pointer" << std::endl;
                    MPI_Finalize();
                    exit(1);
                }
            }
        }
    }

    void print_info () {
        std::cout << "rank: " << rank << ", root: " << root << ", left: " << left
              << ", right: " << right;
        if (marker_pointer == PARENT){
            std::cout << ", marker pointer: PARENT";
        } else if (marker_pointer == LEFT) {
            std::cout << ", marker pointer: LEFT";
        } else if (marker_pointer == RIGHT) {
            std::cout << ", marker pointer: RIGHT";
        }
        std::cout << ", marker: " << marker << std::endl;
    }
};

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int tasks, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int marker_node = atoi(argv[1]) % tasks;
    int marker_cnt = marker_node;
    std::map<int, int> from_root_to_marker;

    while (marker_cnt != TREE_ROOT) {
        from_root_to_marker[(marker_cnt - 1) / 2] = marker_cnt;
        marker_cnt = (marker_cnt - 1) / 2;
    }
    // создание структуры-дерева для каждого процесса
    tree tree_elem;
    if (rank == 0) {
        tree_elem.rank = TREE_ROOT;
        tree_elem.root = -1;
        tree_elem.left = 1;
        tree_elem.right = 2;
        tree_elem.marker_pointer = PARENT;
        tree_elem.marker = marker_node == TREE_ROOT;
    } else {
        int left = 2 * rank + 1;
        if (left >= tasks)
            left = -1;
        int right = 2 * rank + 2;
        if (right >= tasks)
            right = -1;
        tree_elem.rank = rank;
        tree_elem.root = (rank - 1) / 2;
        tree_elem.left = left;
        tree_elem.right = right;
        tree_elem.marker_pointer = PARENT;
        tree_elem.marker = marker_node == rank;
    }
    // заполнение указателей на маркер
    if (from_root_to_marker.count(rank)) {
        int relative_marker_path = from_root_to_marker[rank];
        if (tree_elem.left != -1 && relative_marker_path == tree_elem.left) {
            tree_elem.marker_pointer = LEFT;
        } else if (tree_elem.right != -1 && relative_marker_path == tree_elem.right) {
            tree_elem.marker_pointer = RIGHT;
        }
    }
    // полученное дерево
    tree_elem.print_info();
    // обмен сообщениями и получение маркера
    MPI_Barrier(MPI_COMM_WORLD);
    for (int request_sender = 0; request_sender < tasks; request_sender++) {
        if (rank == request_sender) {
            tree_elem.request(REQUEST, rank);
            message_type tmp_m;
            if (tree_elem.marker_pointer == PARENT) {
                if (rank != 0)
                    MPI_Recv(&tmp_m, 1, MPI_INT8_T, tree_elem.root, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (tree_elem.marker_pointer == LEFT) {
                MPI_Recv(&tmp_m, 1, MPI_INT8_T, tree_elem.left, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (tree_elem.marker_pointer == RIGHT) {
                MPI_Recv(&tmp_m, 1, MPI_INT8_T, tree_elem.right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            if (tmp_m == MARKER) {
                tree_elem.request(MARKER, tree_elem.root);
            } else if (tmp_m == REQUEST) {
                tree_elem.request(REQUEST, tree_elem.root);
            }
            for (int i = 0; i < tasks; i++) {
                if (i != request_sender) {
                    message_type tmp = NOTHING;
                    MPI_Send(&tmp, 1, MPI_INT8_T, i, 0, MPI_COMM_WORLD);
                }
            }
        } else {
            message_type inp;
            do {
                MPI_Status status;
                MPI_Recv(&inp, 1, MPI_INT8_T, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                if (inp == MARKER) {
                    tree_elem.request(MARKER, status.MPI_SOURCE);
                } else if (inp == REQUEST) {
                    tree_elem.request(REQUEST, status.MPI_SOURCE);
                }
            } while (inp != NOTHING);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
