#include "be.h"

void be_optimize(Be_Module* module) {
    bool changed = false;
    do {
        changed = false;
        changed |= be_fold(module);
        be_build_cfg(module);
        changed |= be_simplify_branches(module);
        be_build_cfg(module);
        changed |= be_remove_unreachable(module);
        be_build_cfg(module);
        changed |= be_simplify(module);
    } while (changed);
}
