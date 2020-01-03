#ifndef DUNKOS_PCI_H
#define DUNKOS_PCI_H

#include <util/list.h>

#define PCI_MAX_DEVICES 32
#define PCI_MAX_BUSES 256
#define PCI_MAX_FUNCTIONS 8

enum pci_device_class {
    PCI_DCLASS_UNCLASSIFIED,
    PCI_DCLASS_MASS,
    PCI_DCLASS_NETWORK,
    PCI_DCLASS_DISPLAY,
    PCI_DCLASS_MULTIM,
    PCI_DCLASS_MEMORY,
    PCI_DCLASS_BRIDGE,
    PCI_DCLASS_SIMCOM,
    PCI_DCLASS_BASESYSP,
    PCI_DCLASS_INPUT,
    PCI_DCLASS_DOCK,
    PCI_DCLASS_PROC,
    PCI_DCLASS_SERIALBC,
    PCI_DCLASS_WIRELESS,
    PCI_DCLASS_SMART,
    PCI_DCLASS_SATELLITE,
    PCI_DCLASS_ENCYRPTION,
    PCI_DCLASS_SIGNALPR,
    PCI_DCLASS_PROCACC,
    PCI_DCLASS_NONESSINSTR,
    PCI_DCLASS_COPROC = 0x40,
    PCI_DCLASS_UNASSIGNED = 0xFF
};

struct pci_device {
    struct list_elem group_elem;
    enum pci_device_class class;
    int subclass; /* Each different I/O module will have its own interpretation
                     of this integer. */
    int progif;   /* Each different I/O module will have its own interpretation
                     of this integer. */

    int id;
    int buses[PCI_MAX_FUNCTIONS];
    void *config;
    struct pci_device *bridge;
    const char *devstr;
};

void init_pci(void);
struct pci_device *get_pci_device(enum pci_device_class class, int subclass);

#endif