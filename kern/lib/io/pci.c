#include <stdint.h>
#include <synch.h>
#include <string.h>
#include <asm.h>
#include <stdext.h>
#include <algo.h>
#include <heap.h>
#include <util/hashtable.h>
#include <util/debug.h>
#include <pci.h>

#define PACKED __attribute__((packed))
#define PCI_HEADER_STANDARD 0x00
#define PCI_HEADER_P2P 0x01
#define PCI_HEADER_CARDBUS 0x02
#define PCI_HEADER_MULTIFN (1 << 7)
#define PCI_CONFIG_ADDR  (uint16_t)0x0CF8
#define PCI_CONFIG_DATA  (uint16_t)0x0CFC
#define PDEV_FIND_CFG -2
#define PDEV_NO_CFG -1

struct PACKED rsdp_descriptor {
    char sig[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdtaddr;

    uint32_t length;
    uint64_t xsdtaddr;
    uint8_t extchecksum;
    uint8_t reserved[3];
};

struct PACKED cfg_space_allocation {
    uint64_t ecfgbase;
    uint16_t pciseggnum;
    uint8_t startpcibusn;
    uint8_t endpcibusn;
    uint32_t resvd;
};

struct PACKED mcfg_table {
    char sig[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    uint64_t oemtblid;
    uint32_t oemrevision;
    uint32_t creatorid;
    uint32_t creatorrevision;
    uint64_t resvd;
};

/* Group of PCI devices grouped by category. */
struct pci_device_group {
    struct list devices;
};

static struct pci_device_group device_table[PCI_DCLASS_UNASSIGNED + 1];
static uint16_t pci_config_word(struct cfg_space_allocation *cfg, uint8_t bus, uint8_t device, uint8_t func, uint8_t ofs);
static void enumerate_pci_devices(void);

void init_pci(void)
{
    enum pci_device_class class;
    for (class = 0; class <= PCI_DCLASS_UNCLASSIFIED; ++class) {
        list_init(&device_table[class].devices);
    }

    enumerate_pci_devices();
}

struct pci_device *get_pci_device(enum pci_device_class class, int subclass)
{
    struct pci_device *dev = NULL;
    struct list *dgrlist = &device_table[class].devices;
    if (PCI_DCLASS_UNCLASSIFIED <= class && class <= PCI_DCLASS_UNASSIGNED && 
        (!list_empty(dgrlist))) {
        struct list_elem *el = list_begin(dgrlist);
        dev = elem_value(el, struct pci_device, group_elem);

        if (subclass != -1) {
            while (dev->subclass != subclass && el != list_end(dgrlist)) {
                el = list_next(el);
                dev = elem_value(el, struct pci_device, group_elem);
            }

            if (dev->subclass != subclass) {
                dev = NULL;
            }
        }
    }
    return dev;
}

static struct cfg_space_allocation *cfg_table;
static struct rsdp_descriptor *find_rsdp(void);
static void enumerate_device(int pdev);
static void *find_sdt(void *xsdt, const char *name);
static const char *get_device_string(struct pci_device *dev);

static void enumerate_pci_devices(void)
{
    struct rsdp_descriptor *rsdp = find_rsdp();
    
    assert(rsdp != NULL);
    if (rsdp->revision == 2)
        assert(getbyte(memsum(rsdp, sizeof *rsdp), 0) == 0);
    else
        assert(getbyte(memsum(rsdp, offsetof(struct rsdp_descriptor, length)), 0) == 0);

    void *sdt = (void *)((rsdp->revision == 2) ? (rsdp->xsdtaddr) : (rsdp->rsdtaddr));
    struct mcfg_table *mcfg = find_sdt(sdt, "MCFG");
    
    if (mcfg != NULL) {
        assert(getbyte(memsum(mcfg, mcfg->length), 0) == 0);

        cfg_table = (struct cfg_space_allocation *)(mcfg + 1);
        size_t i, length = (mcfg->length - sizeof *mcfg) / sizeof(struct cfg_space_allocation);
        for (i = 0; i < length; ++i) {
            enumerate_device(i);
        }
    } else {
        enumerate_device(PDEV_NO_CFG);
    }
}

static void init_pci_dev(struct pci_device *pci_dev, struct cfg_space_allocation *cfg, 
                         int bus, int dev, struct pci_device *bridge)
{
    int classdata = pci_config_word(cfg, bus, dev, 0, 5);
    pci_dev->config = cfg;
    pci_dev->class = getbyte(classdata, 1);
    pci_dev->subclass = getbyte(classdata, 0);
    pci_dev->progif = getbyte(pci_config_word(cfg, bus, dev, 0, 4), 1);
    pci_dev->id = pci_config_word(cfg, bus, dev, 0, 1);
    pci_dev->buses[0] = bus;
    pci_dev->bridge = bridge;
    pci_dev->devstr = get_device_string(pci_dev);

    list_push_back(&device_table[pci_dev->class].devices, &pci_dev->group_elem);
}

static void check_bus(struct pci_device *bridge, int bus);

static void check_function(struct pci_device *dev, struct pci_device *bridge, int bus, int device, int function) {
    int classdata;
    int class, subclass, sndbus;

    classdata = pci_config_word(bridge->config, bus, device, function, 5);
    class = getbyte(classdata, 1);
    subclass = getbyte(classdata, 0);

    if (class == PCI_DCLASS_BRIDGE && subclass == 0x04) {
        sndbus = getbyte(pci_config_word(bridge->config, bus, device, function, 12), 1);
        dev->buses[function] = sndbus;
        check_bus(dev, sndbus);
    }
}

static void check_device(struct pci_device *bridge, int bus, int device)
{
    int function = 0;
    if (pci_config_word(bridge->config, bus, device, function, 0) != 0xFFFF) {
        
        struct pci_device *dev = kcalloc(1, sizeof *dev);
        init_pci_dev(dev, bridge->config, bus, device, bridge);
        check_function(dev, bridge, bus, device, function);
        int header_type = getbyte(pci_config_word(bridge->config, bus, device, function, 7), 0);

        if ((header_type & PCI_HEADER_MULTIFN) != 0) {
            for (function = 1; function < PCI_MAX_FUNCTIONS; ++function) {
                if (pci_config_word(bridge->config, bus, device, function, 0) != 0xFFFF) {
                    check_function(dev, bridge, bus, device, function);
                }
            }
        }
    }
}

static void check_bus(struct pci_device *bridge, int bus)
{
    int dev;
    for (dev = 0; dev < PCI_MAX_DEVICES; ++dev) {
        check_device(bridge, bus, dev);
    }
}

static void enumerate_device(int pdev)
{
    struct cfg_space_allocation *cfg = (pdev == PDEV_NO_CFG) ? NULL : (cfg_table + pdev);
    int bus = (cfg != NULL) ? cfg->startpcibusn : 0;
    int header_type = getbyte(pci_config_word(cfg, bus, 0, 0, 7), 0);
    struct pci_device *dev = kcalloc(1, sizeof *dev);
    init_pci_dev(dev, cfg, bus, 0, NULL);

    if ((header_type & PCI_HEADER_MULTIFN) == 0) {
        check_bus(dev, 0);
    } else {
        int function;
        for (function = 0; function < PCI_MAX_FUNCTIONS; ++function) {
            if (pci_config_word(cfg, bus, 0, function, 0) != 0xFFFF) {
                break;
            }

            check_bus(dev, function);
        }
    }
}

static uint16_t pci_config_word(struct cfg_space_allocation *cfg, uint8_t bus, uint8_t device, uint8_t func, uint8_t ofs)
{
    uint32_t config_data;
    uint32_t lbus = bus;
    uint32_t ldev = device;
    uint32_t lfunc = func;

    if (cfg != NULL) {
        uint16_t *cfgwordptr = (uint16_t *)(
            (cfg->ecfgbase)
            + (((lbus - cfg->startpcibusn) << 20) 
            | (ldev << 15) 
            | (lfunc << 12))
        );

        config_data = cfgwordptr[ofs];
    } else {
        uint32_t addr = ((lbus << 16) | (ldev << 11) | (lfunc << 8) | ((ofs * 2) & 0xFC) | 0x80000000);
        uint32_t val;
        out(addr, PCI_CONFIG_ADDR);
        in(PCI_CONFIG_DATA, &val);
        config_data = (uint16_t)((val >> ((ofs & 1) * 16)) & 0xFFFF);
    }

    return config_data;
}

static struct rsdp_descriptor *find_rsdp(void)
{
    const size_t inc = 16;

    uint8_t *ptr = (uint8_t *)(0x000E0000);
    while (ptr != (uint8_t *)(0x000FFFFF)) {
        if (!strncmp((const char *)ptr, "RSD PTR ", 8)) {
            break;
        }
        ptr += inc;
    }

    return (ptr == (uint8_t *)(0x000FFFFF)) ? (NULL) : ((struct rsdp_descriptor *)ptr);
}

struct PACKED sdt_header {
    char sig[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oemtblid[8];
    uint32_t oemrevision;
    uint32_t creatorid;
    uint32_t creatorrevision;
};

static void *find_sdt(void *psdt_, const char *name) {
    struct sdt_header *psdt = psdt_;

    assert(psdt != NULL);
    assert(getbyte(memsum(psdt, psdt->length), 0) == 0);

    char namecmpstr[4];
    memcpy(namecmpstr, name, min(strlen(name), 4));

    void *sdt = NULL;
    struct sdt_header *tbl = psdt + 1;
    size_t i, len = (psdt->length - sizeof *psdt) / (sizeof *psdt);
    for (i = 0; i < len; ++i) {
        if (!strncmp(namecmpstr, tbl[i].sig, 4)) {
            sdt = (tbl + i);
            break;
        }
    }

    return sdt;
}

#define PCI_DEVSTRMAX 128

static const char *get_device_string(struct pci_device *dev)
{
    struct pci_device *bridge = dev->bridge;
    char *dstrbuf = kcalloc(PCI_DEVSTRMAX, sizeof *dstrbuf);
    sprintf(dstrbuf, "pcidev%i[with Bridge=%i,Class=%i,Subclass=%i,ProgIF=%i]", 
             dev->id, (bridge != NULL) ? bridge->id : -1, dev->class, dev->subclass, dev->progif);
    return dstrbuf;
}