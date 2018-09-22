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
                for (int i = 0; i < n_params; i++)
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

/*
void setDomStats(int n_pcpus, struct DomList *dom_list, struct DomStats *dom_stats)
{
    virVcpuInfoPtr pcpu_info_vcpu;
    unsigned char *cpumaps;
    size_t len_cpumap;
    for (int i = 0; i < dom_list->n_doms; i++) {
        int n_vcpus = 0;
        if ((n_vcpus = virDomainGetVcpusFlags(dom_list->doms[i], VIR_DOMAIN_AFFECT_LIVE)) == -1) {
            printf("[ERROR] Domain status changed during calculation.\n");
        }
        dom_stats[i].vcpus_time = (unsigned long long *)calloc(n_vcpus, sizeof(unsigned long long));
        dom_stats[i].pcpus = (int *)calloc(n_vcpus, sizeof(int));
        pcpu_info_vcpu = (virVcpuInfoPtr)calloc(n_vcpus, sizeof(virVcpuInfo));
        len_cpumap = VIR_CPU_MAPLEN(n_pcpus);
        cpumaps = (unsigned char *)calloc(n_vcpus, len_cpumap);
        if (virDomainGetVcpus(dom_list->doms[i], pcpu_info_vcpu, n_vcpus, cpumaps, len_cpumap) == -1) {
            printf("[ERROR] Could not retrieve vCpus affinity pcpu_info\n");
        }
        for (int j = 0; j < n_vcpus; j++) {
            dom_stats[i].vcpus_time[j] = pcpu_info_vcpu[j].cpuTime; //TODO: Consider stores vcpu time and cpu to the slot indexed by "number" field.
            dom_stats[i].pcpus[j] = pcpu_info_vcpu[j].cpu;
        }
        dom_stats[i].dom = dom_list->doms[i];
        dom_stats[i].n_vcpus = n_vcpus;
    }

    return;
}


double usage(unsigned long long int diff, double period)
{
    return diff / period * 100;
}

void clearPcpuUsage(double *pcpu_usage, int n_pcpus)
{
    for (int i = 0; i < n_pcpus; i++)
    {
        pcpu_usage[i] = 0.0;
    }

    return;
}

void vcpuPin(double *pcpu_usage, int n_pcpus, struct DomStats *cur_dom_stats, struct DomStats *prev_dom_stats, int n_doms, double period) {
    int busiest_cpu;
    int freest_cpu;
    double busiest_usage = 0.0;
    double freest_usage = DBL_MAX;
    for (int i = 0; i < n_pcpus; i++) {
        if (pcpu_usage[i] > busiest_usage) {
            busiest_usage = pcpu_usage[i];
            busiest_cpu = i;
        }
        if (pcpu_usage[i] < freest_usage) {
            freest_usage = pcpu_usage[i];
            freest_cpu = i;
        }
    }
    size_t len_cpumap = VIR_CPU_MAPLEN(n_pcpus);
    unsigned char determ_cpumap = 0x1;
    if ((busiest_usage - freest_usage) < DIFF_THRESHOLD) {
        printf("Balanced enough for diff threshold = %f.No need to change pinning, making deterministic pinning...\n", DIFF_THRESHOLD);
        for (int i = 0; i < n_doms; i++) {
            for (int j = 0; j < cur_dom_stats[i].n_vcpus; j++) {
                determ_cpumap = 0x1 << cur_dom_stats[i].pcpus[j];
                virDomainPinVcpu(cur_dom_stats[i].dom, j, &determ_cpumap, len_cpumap);
            }
        }
        return;
    }
    printf("Freest cpu %d has usage %f,\n Busiest cpu %d has usage %f.\n", freest_cpu, freest_usage,
        busiest_cpu, busiest_usage);
    unsigned char freest_cpumap = 0x1 << freest_cpu;
    struct Vcpu lightest_vcpu = {0};  //TODO: check if this initializes..
    double lightest_usage = DBL_MAX;
    double use = 0.0;
    for (int i = 0; i < n_doms; i++) {
        for (int j = 0; j < cur_dom_stats[i].n_vcpus; j++) {
            if (cur_dom_stats[i].pcpus[j] == busiest_cpu) {
                use =  usage(cur_dom_stats[i].vcpus_time[j] -
                             prev_dom_stats[i].vcpus_time[j], period);
                if (use < lightest_usage) {
                    lightest_usage = use;
                    lightest_vcpu.dom = cur_dom_stats[i].dom;
                    lightest_vcpu.vcpu_num = j;
                }
            }
        }
    }
    printf("Pinning vcpu %d in domain %s to cpu %d ....\n", lightest_vcpu.vcpu_num,
           virDomainGetName(lightest_vcpu.dom), freest_cpu);
    if (virDomainPinVcpu(lightest_vcpu.dom, lightest_vcpu.vcpu_num, &freest_cpumap, len_cpumap) == 0) {
        printf("Pinning finished!\n");
    } else {
        printf("Pinning failed!\n");
    };

    return;
}

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

int main()
{
    virConnectPtr connection = virConnectOpen("qemu:///system");

    struct DomainArray active_domains;
    struct PCpuStatsArray pCpus_stats;
    getActiveDomains(connection, &active_domains);
    getPCpuStats(connection, &pCpus_stats);
}
