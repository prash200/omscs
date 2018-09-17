//
// Created by pwu on 9/21/17.
//

#include <stdio.h>
#include <libvirt/libvirt.h>
#include <stdlib.h>
#include <unistd.h>

static const double STARVATION =  0.2; // If unused < 0.2 * actual -> starving
static const double WASTE = 0.35;  // If unused > 0.35 * actual -> wasting
static const double PENALTY = 0.3; // Penalty. By which rate we are deflating the unused memory.
static const double HOST_GENEROSITY = 8.0; // One factor by which memory is released from host
static const unsigned long MIN_HOST_MEMORY = 400 * 1024; //Amount of memory needed by host to keep from crash.
static const unsigned long MIN_DOM_MEMORY = 350 * 1024; // Amount of memory needed by domain to keep from crash.
static const char DBG_PWU = 1;

struct DomList {
    virDomainPtr *doms;
    int n_doms;
};

struct DomLinkedNode {
    virDomainPtr dom;
    struct DomLinkedNode *next;
    unsigned long act_memory;
    unsigned long avail_memory; // kB
};

int activeDoms(virConnectPtr conn, struct DomList *list) {
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                         VIR_CONNECT_LIST_DOMAINS_RUNNING;
    virDomainPtr *doms;
    int n_doms = virConnectListAllDomains(conn, &doms, flags);
    if (n_doms == 0) {
        printf("[ERROR] No active domains!\n");
        exit(EXIT_FAILURE);
    }
    list->doms = doms;
    list->n_doms = n_doms;
    if (DBG_PWU) {
        printf("---------we have %d active domains--------\n", n_doms);
    }
    return n_doms;
}

void setUpNode(struct DomLinkedNode **head, struct DomLinkedNode **cur, virDomainPtr dom,
               unsigned long act_memory, unsigned long avail_memory) {
    if (!(*head)) {
        *head = (struct DomLinkedNode *)calloc(1, sizeof(struct DomLinkedNode));
        *cur = *head;
    } else {
        struct DomLinkedNode *newNode = (struct DomLinkedNode *)calloc(1, sizeof(struct DomLinkedNode));
        (*cur)->next = newNode;
        (*cur) = (*cur)->next;
    }
    (*cur)->next = NULL;
    (*cur)->dom = dom;
    (*cur)->act_memory = act_memory;
    (*cur)->avail_memory = avail_memory;
    return;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("[ERROR] Usage: memory_coordinator <time interval(s)>.\n");
        return 1;
    }
    virConnectPtr conn = virConnectOpen("qemu:///system");
    struct DomList* dom_list = (struct DomList *)calloc(1, sizeof(struct DomList)); //TODO: free
    if (!dom_list) {
        printf("[ERROR] Cannot allocate memory.\n");
        return(1);
    }
    while (activeDoms(conn, dom_list) > 0) {
        struct DomLinkedNode *starvHead = NULL, *starvCur = NULL, *wasteHead = NULL, *wasteCur = NULL, *cur = NULL;
        int n_nodes[2] = {0};
        //Form linked list of starving doms, wasting doms.
        for (int i = 0; i < dom_list->n_doms; i++) {
            virDomainMemoryStatStruct mem_stats[VIR_DOMAIN_MEMORY_STAT_NR];
            unsigned int flags = VIR_DOMAIN_AFFECT_CURRENT;
            if (virDomainSetMemoryStatsPeriod(dom_list->doms[i], 1, flags)
                == -1) {
                printf("[ERROR] %sCould not change balloon collect period for domain.\n",
                       virDomainGetName(dom_list->doms[i]));
            }
            if (virDomainMemoryStats(dom_list->doms[i], mem_stats,
            VIR_DOMAIN_MEMORY_STAT_NR, 0) == -1) {
                printf("[ERROR] %s: Could not collect memory stats of domain.\n",
                    virDomainGetName(dom_list->doms[i]));
            }
            unsigned long long actual;
            unsigned long long available;
            unsigned long long unused;
            for (int j = 0; j < VIR_DOMAIN_MEMORY_STAT_NR; j++) {
                if (mem_stats[j].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON) {
                    actual = mem_stats[j].val;
                } else if (mem_stats[j].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE) {
                    available = mem_stats[j].val;
                } else if (mem_stats[j].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED) {
                    unused = mem_stats[j].val;
                }

            }
            //TODO: remove
            printf("%s: actual balloon size--%luMB;\tavailable--%luMB;\tunused--%luMB.\n", virDomainGetName(dom_list->doms[i]),
                   (unsigned long)(actual / 1024),
                   (unsigned long)(available / 1024),
                   (unsigned long)(unused / 1024));
           if (unused < STARVATION * actual) {
                setUpNode(&starvHead, &starvCur, dom_list->doms[i],
                          actual,
                          unused);
                n_nodes[0]++;
            } else if (unused > WASTE * actual) {
                setUpNode(&wasteHead, &wasteCur, dom_list->doms[i],
                          actual,
                          unused);
                n_nodes[1]++;
            }
        }
        printf("Number of starving domains: %d;\nNumber of wasteful domains: %d.\n", n_nodes[0],n_nodes[1]);
        if (starvHead) {
            //There are starving domains.
            unsigned long totalNeed = 0; //kB
            cur = starvHead;
            for (int i = 0; i < n_nodes[0]; i++) {
                totalNeed += (STARVATION * cur->act_memory) - cur->avail_memory;
                cur = cur->next;
            }
            printf("Starving domains need %lu MB in total.\n", totalNeed / 1024);
            unsigned long totalRelease = 0; //kB
            if (wasteHead) {
                cur = wasteHead;
                for (int i = 0; i < n_nodes[1]; i++) {
                    unsigned long alloc = cur->act_memory - cur->avail_memory * PENALTY;
                    if (alloc < MIN_DOM_MEMORY) {
                        alloc = MIN_DOM_MEMORY;
                    }
                    virDomainSetMemory(cur->dom, alloc);
                    totalRelease += (cur->act_memory - alloc);
                    printf("Wasteful domain %s: Deflating memory from %lu MB to %lu MB.\n",
                           virDomainGetName(cur->dom), cur->act_memory / 1024, alloc / 1024);
                    cur = cur->next;
                }
            }
            printf("Total release by wasteful vms is %lu.\n", totalRelease / 1024);
            if (totalRelease < totalNeed) {
                unsigned long nodeProv = HOST_GENEROSITY * (totalNeed - totalRelease);
                unsigned long nodeFree = virNodeGetFreeMemory(conn) / 1024 - MIN_HOST_MEMORY;
                if (nodeFree <= 0) {
                    printf("[Warning] host memory not enough. Will not provide memory from house this time.\n");
                } else {
                    if (nodeProv > nodeFree) {
                        nodeProv = nodeFree;
                    }
                    totalRelease += nodeProv;
                    printf("Total release after adding host is %lu.\n", totalRelease / 1024);
                }
            }
            unsigned long provision = totalRelease / n_nodes[0];
            cur = starvHead;
            for (int i = 0; i < n_nodes[0]; i++) {
                unsigned long alloc = cur->act_memory + provision;
                unsigned long maxMemory = virDomainGetMaxMemory(cur->dom);
                if (alloc > maxMemory) {
                    alloc = maxMemory; // If you really need that much, let me go ahead and allocate the most possibly
                }
                virDomainSetMemory(cur->dom, alloc);
                printf("Starving domain %s: Inflating Memory from %lu MB to %lu MB.\n", virDomainGetName(cur->dom), cur->act_memory / 1024, alloc / 1024);
                cur = cur->next;
            }
        } else if (wasteHead) {
            // No starving domain, give back some wasted memory to host.
            printf("No starving domain. Putting back memory to host.\n");
            cur = wasteHead;
            for (int i = 0; i < n_nodes[1]; i++) {
                unsigned long alloc = cur->act_memory - cur->avail_memory * PENALTY;
                if (alloc < MIN_DOM_MEMORY) {
                    alloc = MIN_DOM_MEMORY;
                }
                virDomainSetMemory(cur->dom, alloc);
                printf("Wasteful domain %s: Deflating memory %lu MB to %lu MB to host.\n",
                       virDomainGetName(cur->dom), cur->act_memory / 1024, alloc / 1024);
                cur = cur->next;
            }
        }
        printf("Finished coordination!\n\n\n\n");
        fflush(stdout);
        sleep(atoi(argv[1]));
    } // Finish while loop.
    printf("No active doms. Program exiting...");
    virConnectClose(conn);
    return 0;
}
