#include <stdio.h>
#include <libvirt/libvirt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <float.h>

#ifdef DEBUG
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

static const long NANOSECOND = 1000000000;
static const double DIFF_THRESHOLD = 20.0;

// Represents an array of domain pointers
struct DomainArray
{
    // Pointer to an array of domain pointers
    virDomainPtr *domains;
    // No. of domains in the domain
    int n_domains;
};

// Represents pCPU stats
struct PCpuStats
{
    // pCPU id
    int pCpu_id;
    // load on pCPU
    unsigned long long load;
};

// Represents a pCPU stats Array
struct PCpuStatsArray
{
    // Pointer to an array of pCPUs
    struct PCpuStats *pCpus_stats;
    // No. of pCPUs
    int n_pCpus;
};

// Represents vCPU
struct VCpuStats
{
    // Domian pointer
    virDomainPtr domain;
    // vCPU id
    int vCpu_id;
    // load on vCPU
    unsigned long long load;
};

// Represents a vCPU Array
struct VCpuStatsArray
{
    // Pointer to an array of vCPUs
    struct VCpuStats *vCpus_stats;
    // No. of vCPUs
    int n_vCpus;
};

// Trace pCpustats array
void tracePCpuStatsArray(struct PCpuStatsArray* pCpus_stats)
{
    #ifdef DEBUG
    for (int i = 0 ; i < pCpus_stats->n_pCpus ; ++i)
    {
        printf("id = %d, load = %llu\n", pCpus_stats->pCpus_stats[i].pCpu_id, pCpus_stats->pCpus_stats[i].load);
    }
    #endif
}

// Trace vCpustats array
void traceVCpuStatsArray(struct VCpuStatsArray* vCpus_stats)
{
    #ifdef DEBUG
    for (int i = 0 ; i < vCpus_stats->n_vCpus ; ++i)
    {
        printf("id = %d, load = %llu\n", vCpus_stats->vCpus_stats[i].vCpu_id, vCpus_stats->vCpus_stats[i].load);
    }
    #endif
}

//  Gets an array of active domains
void getActiveDomains(virConnectPtr connection, struct DomainArray *domain_array)
{
    TRACE("getActiveDomains called\n");
    TRACE("connection = %p\n", connection);
    TRACE("domain_array = %p\n", domain_array);

    virDomainPtr *domains = NULL;
    int n_domains = virConnectListAllDomains(connection, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING);

    TRACE("domains = %p\n", domains);
    TRACE("domains = %d\n", n_domains);

    if (n_domains == 0)
    {
        printf("[ERROR] No active domains!\n");
        exit(EXIT_FAILURE);
    }

    domain_array->domains = domains;
    domain_array->n_domains = n_domains;
}

//  Gets pCPU stats
void getPCpuStats(virConnectPtr connection, struct PCpuStatsArray *pCpus_stats)
{
    TRACE("getPCpuStats called\n");
    TRACE("connection = %p\n", connection);
    TRACE("pCpus_stats = %p\n", pCpus_stats);

    virNodeInfo pcpu_info;
    virNodeGetInfo(connection, &pcpu_info);

    TRACE("n_pCpus = %u\n", pcpu_info.cpus);

    struct PCpuStats *pCpu_stats = malloc(sizeof(struct PCpuStats) * pcpu_info.cpus);

    TRACE("pCpu_stats = %p\n", pCpu_stats);

    for (int cpu_no = 0 ; cpu_no < pcpu_info.cpus ; ++cpu_no)
    {
        TRACE("cpu_no = %d\n", cpu_no);

        int n_params = 0;
        if (virNodeGetCPUStats(connection, cpu_no, NULL, &n_params, 0) == 0 && n_params != 0)
        {
            TRACE("n_params = %d\n", n_params);

            virNodeCPUStats *params = malloc(sizeof(virNodeCPUStats) * n_params);
            
            TRACE("params = %p\n", params);

            if (params != NULL)
            {
                memset(params, 0, sizeof(virNodeCPUStats) * n_params);
            }
            
            if (virNodeGetCPUStats(connection, cpu_no, params, &n_params, 0) == 0)
            {
                unsigned long long load = 0;
                for (int i = 0; i < n_params; ++i)
                {
                    if (strcmp(params[i].field, VIR_NODE_CPU_STATS_USER) == 0 || strcmp(params[i].field, VIR_NODE_CPU_STATS_KERNEL) == 0)
                    {
                        TRACE("params[%d].field = %s, params[%d].value = %llu\n", i, params[i].field, i, params[i].value);
                        load += params[i].value;
                    }
                }

                TRACE("load = %llu\n", load);

                pCpu_stats[cpu_no].pCpu_id = cpu_no;
                pCpu_stats[cpu_no].load = load;
            }

            free(params);
        }
    }
    
    pCpus_stats->pCpus_stats = pCpu_stats;
    pCpus_stats->n_pCpus = pcpu_info.cpus;
}

//  Gets vCPU stats
void getVCpuStats(struct DomainArray *active_domains, struct VCpuStatsArray *vCpus_stats)
{
    TRACE("getVCpuStats called\n");
    TRACE("active_domains = %p\n", active_domains);
    TRACE("vCpus_stats = %p\n", vCpus_stats);

    // Assuming only one vCPU per VM
    struct VCpuStats *vCpu_stats = malloc(sizeof(struct VCpuStats) * active_domains->n_domains);
    TRACE("vCpu_stats = %p\n", vCpu_stats);

    for (int i = 0 ; i < active_domains->n_domains ; ++i)
    {
        // Assuming only one vCPU per VM
        virVcpuInfoPtr info = (virVcpuInfoPtr)malloc(sizeof(virVcpuInfo));
        virDomainGetVcpus(active_domains->domains[i], info, 1, NULL, 0);

        TRACE("info->number = %d, info->cputime = %llu, info->state = %d, info->cpu = %d\n", info->number, info->cpuTime, info->state, info->cpu);

        vCpu_stats[i].domain = active_domains->domains[i];
        vCpu_stats[i].load = info->cpuTime;
        vCpu_stats[i].vCpu_id = info->number;

        free (info);
    }

    vCpus_stats->vCpus_stats = vCpu_stats;
    vCpus_stats->n_vCpus = active_domains->n_domains;
}

// bubble down heap w.r.t. pcpu load
void bubbleDownHeap(struct PCpuStats *pCpu_stats, int index, int heap_size)
{
    TRACE("pCpu_stats = %p, index = %d, heap_size = %d\n", pCpu_stats, index, heap_size);

    int left = (2*index+1);
    if (left >= heap_size)
    {
        return;
    }
    else
    {
        int smallest_index = heap_size;
        if (left < heap_size && pCpu_stats[left].load < pCpu_stats[index].load)
        {
            smallest_index = left;
        }
        else
        {
            smallest_index = index;
        }

        int right = ((2*index)+2);
        if (right < heap_size && pCpu_stats[right].load < pCpu_stats[smallest_index].load)
        {
            smallest_index = right;
        }

        if (smallest_index != index)
        {
            struct PCpuStats temp = pCpu_stats[index];
            pCpu_stats[index] = pCpu_stats[smallest_index];
            pCpu_stats[smallest_index] = temp;
            bubbleDownHeap(pCpu_stats, smallest_index, heap_size);
        }
    }
}

// bubble up heap w.r.t. pcpu load
void bubbleUpHeap(struct PCpuStats *pCpu_stats, int index)
{
    TRACE("pCpu_stats = %p, index = %d\n", pCpu_stats, index);

    if (index == 0)
    {
        return;
    }

    int parent = (index-1)/2;
    if (pCpu_stats[parent].load > pCpu_stats[index].load)
    {
        struct PCpuStats temp = pCpu_stats[index];
        pCpu_stats[index] = pCpu_stats[parent];
        pCpu_stats[parent] = temp;
        bubbleUpHeap(pCpu_stats, parent);
    }
}

// build min heap w.r.t. pcpu load
void buildHeap(struct PCpuStatsArray* pCpus_stats)
{
    TRACE("buildMinHeap called\n");
    tracePCpuStatsArray(pCpus_stats);

    int heap_size = pCpus_stats->n_pCpus;
    for (int j = heap_size/2 ; j >= 0 ; j--)
    {
        bubbleDownHeap(pCpus_stats->pCpus_stats, j, heap_size);
    }

    tracePCpuStatsArray(pCpus_stats);
}

// Delete min value from the heap
int deleteHeapMin(struct PCpuStatsArray* pCpus_stats)
{
    TRACE("extractMinIndex called\n");
    tracePCpuStatsArray(pCpus_stats);

    int heap_size = pCpus_stats->n_pCpus;
    if (heap_size == 0)
    {
        return;
    }

    pCpus_stats->n_pCpus -= 1;
    if (heap_size == 1)
    {
        return;
    }
    
    struct PCpuStats temp = pCpu_stats->pCpu_stats[0];
    pCpu_stats->pCpu_stats[0] = pCpu_stats->pCpu_stats[heap_size-1];
    pCpu_stats->pCpu_stats[heap_size-1] = temp;
    bubbleDownHeap(pCpu_stats, 0, heap_size-1);
    
    tracePCpuStatsArray(pCpus_stats);
}

// Delete min value from the heap
void insertHeap(struct PCpuStatsArray* pCpus_stats, struct PCpuStats pCpu_stats)
{
    TRACE("insertHeap called\n");
    tracePCpuStatsArray(pCpus_stats);

    int heap_size = pCpus_stats->n_pCpus;
    pCpus_stats->n_pCpus += 1;
    pCpus_stats->pCpus_stats[heap_size] = pCpu_stats;
	bubbleUpHeap(pCpus_stats->pCpus_stats, heap_size);

	tracePCpuStatsArray(pCpus_stats);
}

// Sort w.r.t. vcpu load
void sort(struct  VCpuStatsArray* vCpus_stats)
{
    TRACE("sort called\n");
    traceVCpuStatsArray(vCpus_stats);

    for (int i = 0 ; i < vCpus_stats->n_vCpus ; ++i)
    {
        for (int j = i + 1 ; j < vCpus_stats->n_vCpus ; ++j)
        {
            if (vCpus_stats->vCpus_stats[i].load > vCpus_stats->vCpus_stats[j].load)
            {
                struct VCpuStats temp = vCpus_stats->vCpus_stats[i];
                vCpus_stats->vCpus_stats[i] = vCpus_stats->vCpus_stats[j];
                vCpus_stats->vCpus_stats[j] = temp;
            }
        }
    }

    traceVCpuStatsArray(vCpus_stats);
}

/*
int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("[ERROR] Usage: vcpu_scheduler <time interval(s)>.\n");
        return 1;
    }
    virConnectPtr conn = virConnectOpen("qemu:///system");
    int prev_n_doms = 0;
    int n_pcpus = getNrPcpus(conn);
    struct DomList* dom_list = (struct DomList *)calloc(1, sizeof(struct DomList)); //TODO: free
    double *pcpu_usage = (double *)calloc(n_pcpus, sizeof(double)); //TODO:free
    if (!dom_list || !pcpu_usage) {
        printf("[ERROR] Cannot allocate memory.\n");
        return(1);
    }

    struct DomStats *cur_dom_stats, *prev_dom_stats; //TODO: free inside and outside.
    double period = atof(argv[1]) * NANOSECOND;
    while (activeDoms(conn, dom_list) > 0) {
        // Set up current round data.
        cur_dom_stats = (struct DomStats *)calloc(dom_list->n_doms, sizeof(struct DomStats)); //TODO: free
        setDomStats(n_pcpus, dom_list, cur_dom_stats);

        // Check if # of active domain changes. When unchanged, calculate the usage and pin accordingly.
        if (dom_list->n_doms != prev_n_doms) {
            printf("Number of DOMs changed...\n");
            prev_dom_stats = (struct DomStats *)calloc(dom_list->n_doms, sizeof(struct DomStats));
        } else {
            printf("Number of DOMs stays the same...Calculating...\n");
            double use = 0.0;
            for (int i = 0; i < dom_list->n_doms; i++) {
                for (int j = 0; j < cur_dom_stats[i].n_vcpus; j++) {
                    use =  usage(cur_dom_stats[i].vcpus_time[j] -
                                                prev_dom_stats[i].vcpus_time[j], period);
                    printf("Domain %s 's %d vcpu has usage %f during this period, \n",
                           virDomainGetName(dom_list->doms[i]), j, use);
                    pcpu_usage[cur_dom_stats[i].pcpus[j]] += use;
                }
            }
            vcpuPin(pcpu_usage, n_pcpus, cur_dom_stats, prev_dom_stats, dom_list->n_doms, period);
        } // finish checking dom number change.
        printf("Finished scheduling!\n\n\n\n");
        // Update prev value to cur.
        memcpy(prev_dom_stats, cur_dom_stats, dom_list->n_doms * sizeof(struct DomStats));
        prev_n_doms = dom_list->n_doms;
        clearPcpuUsage(pcpu_usage, n_pcpus);
        sleep(atoi(argv[1]));
    } // finish while loop

    virConnectClose(conn);

    free(dom_list->doms);
    free(dom_list);
    printf("No active doms. Program exiting..\n");
    return 0;
}
*/

void balanceLoad(struct VCpuStatsArray *vCpus_stats, struct PCpuStatsArray *pCpus_stats)
{
    buildHeap(pCpus_stats);
    sort(vCpus_stats);

    for (int i = 0 ; i < n_vCpus ; ++i)
    {
        struct PCpuStats pCpu_stats = pCpus_stats->pCpus_stats[0];
		deleteHeapMin(pCpus_stats);

        //unsigned char pcup_map = 0x1 << pCpu_stats->pCpu_id;
        //virDomainPinVcpu(vCpu_stats[i].domain, vCpu_stats[i].vCpu_id, &pcup_map, VIR_CPU_MAPLEN(pCpu_stats->n_pCpus));

        pCpu_stats.load += vCpu_stats[i].load;
        insertHeap(pCpus_stats, pCpu_stats);
    }
}

int main()
{
    virConnectPtr connection = virConnectOpen("qemu:///system");

    struct DomainArray active_domains;
    struct PCpuStatsArray pCpus_stats;
    struct VCpuStatsArray vCpus_stats;

    getActiveDomains(connection, &active_domains);
    getPCpuStats(connection, &pCpus_stats);
    getVCpuStats(&active_domains, &vCpus_stats);

    balanceLoad(&vCpus_stats, &pCpus_stats);

    return 0;
}
