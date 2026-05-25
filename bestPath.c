#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CARS 513

/* ─── Data structures ─────────────────────────────────────────────────────── */

/* A service station on the highway, identified by its distance (km) from the
   start.  The 'cars' array is a 1-indexed max-heap of vehicle ranges; cars[0]
   stores the current number of vehicles. */
typedef struct {
    int km;
    int *cars;
} Station;

/* Singly-linked list node used to reconstruct a route in order. */
typedef struct Node {
    int val;
    struct Node *next;
} Node;

/* ─── Function prototypes ─────────────────────────────────────────────────── */

void  build_max_heap(int heap[]);
void  max_heapify(int heap[], int i);
void  sift_up(int heap[], int i);
int   find_insert_pos(Station stations[], int lo, int hi, int km);
int   find_station(Station stations[], int lo, int hi, int km);
void  list_push_front(Node **head, int value);

/* ─── Main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    FILE *fin  = stdin;
    FILE *fout = stdout;

    if (fin == NULL || fout == NULL) {
        fprintf(stderr, "Error: cannot open I/O streams.\n");
        return 1;
    }

    /* Dynamic array of stations, kept sorted by km. */
    int capacity    = 10;
    int count       = 0;
    Station *stations = malloc(capacity * sizeof(Station));

    char command[20];

    while (fscanf(fin, "%s", command) != EOF) {

        /* ── ADD STATION ─────────────────────────────────────────────── */
        if (strcmp(command, "aggiungi-stazione") == 0) {
            int km, num_cars;
            fscanf(fin, "%d %d", &km, &num_cars);

            /* Grow the array if needed. */
            if (count >= capacity) {
                capacity += 10;
                stations = realloc(stations, capacity * sizeof(Station));
            }

            /* Find the sorted-insertion position (returns -1 if duplicate). */
            int pos = (count == 0)
                        ? 0
                        : find_insert_pos(stations, 0, count - 1, km);

            if (pos != -1) {
                /* Shift elements right to make room. */
                for (int i = count; i > pos; i--)
                    stations[i] = stations[i - 1];

                /* Initialise the new station and read vehicle ranges. */
                stations[pos].km   = km;
                stations[pos].cars = malloc((MAX_CARS + 1) * sizeof(int));
                stations[pos].cars[0] = num_cars;
                for (int i = 1; i <= num_cars; i++)
                    fscanf(fin, "%d", &stations[pos].cars[i]);

                build_max_heap(stations[pos].cars);

                count++;
                fprintf(fout, "aggiunta\n");
            } else {
                /* Station already exists – discard the car data from input. */
                int dummy;
                for (int i = 0; i < num_cars; i++)
                    fscanf(fin, "%d", &dummy);
                fprintf(fout, "non aggiunta\n");
            }
        }

        /* ── ADD CAR ─────────────────────────────────────────────────── */
        else if (strcmp(command, "aggiungi-auto") == 0) {
            int km, range;
            fscanf(fin, "%d %d", &km, &range);

            int pos = find_station(stations, 0, count, km);
            if (pos != -1) {
                int n = stations[pos].cars[0];

                /* Shift existing cars right and insert the new one at index 1. */
                for (int i = n; i >= 1; i--)
                    stations[pos].cars[i + 1] = stations[pos].cars[i];

                stations[pos].cars[1] = range;
                stations[pos].cars[0] = n + 1;

                /* Restore the max-heap property from the root. */
                max_heapify(stations[pos].cars, 1);

                fprintf(fout, "aggiunta\n");
            } else {
                fprintf(fout, "non aggiunta\n");
            }
        }

        /* ── DEMOLISH STATION ────────────────────────────────────────── */
        else if (strcmp(command, "demolisci-stazione") == 0) {
            int km;
            fscanf(fin, "%d", &km);

            int pos = find_station(stations, 0, count, km);
            if (pos != -1) {
                free(stations[pos].cars);

                /* Shift elements left to fill the gap. */
                for (int i = pos; i < count - 1; i++)
                    stations[i] = stations[i + 1];

                count--;
                fprintf(fout, "demolita\n");
            } else {
                fprintf(fout, "non demolita\n");
            }
        }

        /* ── SCRAP CAR ───────────────────────────────────────────────── */
        else if (strcmp(command, "rottama-auto") == 0) {
            int km, range;
            fscanf(fin, "%d %d", &km, &range);

            int pos   = find_station(stations, 0, count, km);
            int found = 0;

            if (pos != -1) {
                int *heap = stations[pos].cars;
                int  n    = heap[0];

                /* Linear scan to find the car with the given range. */
                for (int i = 1; i <= n && !found; i++) {
                    if (heap[i] == range) {
                        /* Swap with the last element and shrink the heap. */
                        int tmp  = heap[i];
                        heap[i]  = heap[n];
                        heap[n]  = tmp;

                        if (i != 1) {
                            if (heap[i] > heap[n]) {
                                sift_up(heap, i);
                                heap[0]--;
                            } else {
                                heap[0]--;
                                max_heapify(heap, i);
                            }
                        } else {
                            heap[0]--;
                            max_heapify(heap, 1);
                        }
                        found = 1;
                    }
                }

                fprintf(fout, found ? "rottamata\n" : "non rottamata\n");
            } else {
                fprintf(fout, "non rottamata\n");
            }
        }

        /* ── PLAN ROUTE ──────────────────────────────────────────────── */
        else if (strcmp(command, "pianifica-percorso") == 0) {
            int start_km, end_km;
            fscanf(fin, "%d %d", &start_km, &end_km);

            int si = find_station(stations, 0, count, start_km);   /* start index */
            int ei = find_station(stations, 0, count, end_km);     /* end   index */

            /* hop_count[i] = minimum number of hops from start to station i.
               prev[i]      = index of the predecessor station on the best path. */
            int *hop_count = calloc(count, sizeof(int));
            int *prev      = calloc(count, sizeof(int));

            /* ── Forward direction (start < end) ─────────────────────── */
            if (si < ei) {
                int span = ei - si;
                int reach = 1;   /* how many stations ahead the current one can cover */

                for (int i = 0; i < span; i++) {
                    if (reach == 0) reach = 1;

                    int idx = si + i;

                    if (stations[idx].cars[0] == 0) {
                        /* No cars available at this station. */
                        reach = 0;
                        continue;
                    }

                    int max_range = stations[idx].cars[1];  /* heap root = best car */
                    int farthest  = stations[idx].km + max_range;

                    if (farthest >= stations[idx + reach].km) {
                        /* Expand reach as far as possible. */
                        while (idx + reach <= ei &&
                               farthest >= stations[idx + reach].km)
                            reach++;
                        reach--;
                    } else {
                        /* Shrink reach until we find a reachable station. */
                        while (farthest < stations[idx + reach].km)
                            reach--;
                    }

                    if (reach != 0) {
                        /* Update hop counts for every reachable station. */
                        for (int j = 1; j <= reach; j++) {
                            int target = idx + j;
                            int new_hops = hop_count[idx] + 1;

                            if ((hop_count[target] != 0 && new_hops < hop_count[target])
                                || hop_count[target] == 0) {
                                hop_count[target] = new_hops;
                                prev[target] = idx;
                            }
                        }
                    }
                    reach--;
                }

                /* Reconstruct the path using a linked list (front-insertion). */
                Node *head = NULL;
                int cur = ei;
                list_push_front(&head, stations[ei].km);

                while (cur != si && hop_count[cur] != 0) {
                    list_push_front(&head, stations[prev[cur]].km);
                    cur = prev[cur];
                }

                if (cur == si) {
                    /* Print the path. */
                    for (Node *n = head; n != NULL; n = n->next) {
                        fprintf(fout, "%d", n->val);
                        if (n->next) fprintf(fout, " ");
                    }
                    fprintf(fout, "\n");
                } else {
                    fprintf(fout, "nessun percorso\n");
                }

                /* Free the linked list. */
                while (head) { Node *tmp = head; head = head->next; free(tmp); }
            }

            /* ── Backward direction (start > end) ────────────────────── */
            else if (si > ei) {
                int span = si - ei;
                int reach = 1;

                for (int i = 0; i < span; i++) {
                    if (reach == 0) reach = 1;

                    int idx = si - i;

                    if (stations[idx].cars[0] == 0) {
                        reach = 0;
                        continue;
                    }

                    int max_range = stations[idx].cars[1];
                    int farthest  = stations[idx].km - max_range;

                    if (farthest <= stations[idx - reach].km) {
                        /* Expand reach backwards. */
                        while (idx - reach >= ei &&
                               farthest <= stations[idx - reach].km)
                            reach++;
                        reach--;
                    } else {
                        /* Shrink reach until reachable. */
                        while (farthest > stations[idx - reach].km &&
                               idx - reach <= si)
                            reach--;
                    }

                    if (reach != 0) {
                        for (int j = 1; j <= reach; j++) {
                            int target = idx - j;
                            int new_hops = hop_count[idx] + 1;

                            if ((hop_count[target] != 0 && new_hops <= hop_count[target])
                                || hop_count[target] == 0) {
                                hop_count[target] = new_hops;
                                prev[target] = idx;
                            }
                        }
                    }
                    reach--;
                }

                /* Reconstruct the backward path. */
                Node *head = NULL;
                int cur = ei;
                list_push_front(&head, stations[ei].km);

                while (cur != si && hop_count[cur] != 0) {
                    list_push_front(&head, stations[prev[cur]].km);
                    cur = prev[cur];
                }

                if (cur == si) {
                    for (Node *n = head; n != NULL; n = n->next) {
                        fprintf(fout, "%d", n->val);
                        if (n->next) fprintf(fout, " ");
                    }
                    fprintf(fout, "\n");
                } else {
                    fprintf(fout, "nessun percorso\n");
                }

                while (head) { Node *tmp = head; head = head->next; free(tmp); }
            }

            /* ── Same station (start == end) ─────────────────────────── */
            else {
                fprintf(fout, "%d\n", start_km);
            }

            free(hop_count);
            free(prev);
        }
    }

    /* Clean up. */
    for (int i = 0; i < count; i++)
        free(stations[i].cars);
    free(stations);

    fclose(fin);
    fclose(fout);
    return 0;
}

/* ─── Max-heap utilities ──────────────────────────────────────────────────── *
 *
 * The heap is stored in a 1-indexed array where element [0] is the size.
 * The root (index 1) always holds the maximum vehicle range at a station,
 * allowing O(1) lookup of the best car available.
 * ──────────────────────────────────────────────────────────────────────────── */

/* Build a max-heap in-place from an unsorted array. */
void build_max_heap(int heap[])
{
    for (int i = heap[0] / 2 + 1; i >= 1; i--)
        max_heapify(heap, i);
}

/* Push element at index i down until the max-heap property is restored. */
void max_heapify(int heap[], int i)
{
    int left  = 2 * i;
    int right = 2 * i + 1;

    int largest = i;
    if (left  <= heap[0] && heap[left]  > heap[largest]) largest = left;
    if (right <= heap[0] && heap[right] > heap[largest]) largest = right;

    if (largest != i) {
        int tmp       = heap[i];
        heap[i]       = heap[largest];
        heap[largest] = tmp;
        max_heapify(heap, largest);
    }
}

/* Bubble element at index i up toward the root (used after a swap during
   car removal). */
void sift_up(int heap[], int i)
{
    int parent = i / 2;
    if (parent >= 1 && heap[parent] < heap[i]) {
        int tmp      = heap[parent];
        heap[parent] = heap[i];
        heap[i]      = tmp;
        sift_up(heap, parent);
    }
}

/* ─── Binary search utilities ─────────────────────────────────────────────── */

/* Find the correct insertion index for a new station at distance 'km'.
   Returns -1 if a station at that distance already exists. */
int find_insert_pos(Station stations[], int lo, int hi, int km)
{
    if (hi <= lo) {
        if (km == stations[lo].km || km == stations[hi].km)
            return -1;
        if (hi < 0)
            return lo;
        return (km > stations[lo].km) ? lo + 1 : lo;
    }

    int mid = (lo + hi) / 2;
    if (km == stations[mid].km) return -1;
    if (km <  stations[mid].km) return find_insert_pos(stations, lo, mid - 1, km);
    return find_insert_pos(stations, mid + 1, hi, km);
}

/* Find the index of the station at exactly 'km'.
   Returns -1 if no such station exists. */
int find_station(Station stations[], int lo, int hi, int km)
{
    if (hi < lo) return -1;

    int mid = (lo + hi) / 2;
    if (km == stations[mid].km) return mid;
    if (km <  stations[mid].km) return find_station(stations, lo, mid - 1, km);
    return find_station(stations, mid + 1, hi, km);
}

/* ─── Linked-list helper ──────────────────────────────────────────────────── */

/* Insert a new node at the front of the list (used to build the route in
   reverse order so it prints start → end). */
void list_push_front(Node **head, int value)
{
    Node *node = malloc(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        return;
    }
    node->val  = value;
    node->next = *head;
    *head = node;
}