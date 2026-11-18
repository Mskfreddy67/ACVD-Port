// HLE: cellSpurs
#include "runtime/hle.h"

namespace rt {
namespace {

}  // namespace

static hle::Module s_module("cellSpurs", {
    HLE_TODO(_cellSpursAttributeInitialize),
    HLE_TODO(_cellSpursEventFlagInitialize),
    HLE_TODO(_cellSpursJobChainAttributeInitialize),
    HLE_TODO(_cellSpursLFQueueInitialize),
    HLE_TODO(_cellSpursLFQueuePushBody),
    HLE_TODO(_cellSpursSendSignal),
    HLE_TODO(_cellSpursTaskAttributeInitialize),
    HLE_TODO(_cellSpursTasksetAttributeInitialize),
    HLE_TODO(cellSpursAttachLv2EventQueue),
    HLE_TODO(cellSpursAttributeSetNamePrefix),
    HLE_TODO(cellSpursCreateJobChainWithAttribute),
    HLE_TODO(cellSpursCreateTask),
    HLE_TODO(cellSpursCreateTaskWithAttribute),
    HLE_TODO(cellSpursCreateTaskset),
    HLE_TODO(cellSpursCreateTasksetWithAttribute),
    HLE_TODO(cellSpursDetachLv2EventQueue),
    HLE_TODO(cellSpursEventFlagAttachLv2EventQueue),
    HLE_TODO(cellSpursEventFlagClear),
    HLE_TODO(cellSpursEventFlagDetachLv2EventQueue),
    HLE_TODO(cellSpursEventFlagSet),
    HLE_TODO(cellSpursEventFlagWait),
    HLE_TODO(cellSpursFinalize),
    HLE_TODO(cellSpursInitialize),
    HLE_TODO(cellSpursInitializeWithAttribute),
    HLE_TODO(cellSpursJobGuardInitialize),
    HLE_TODO(cellSpursJobGuardNotify),
    HLE_TODO(cellSpursJoinJobChain),
    HLE_TODO(cellSpursJoinTaskset),
    HLE_TODO(cellSpursLFQueueAttachLv2EventQueue),
    HLE_TODO(cellSpursLFQueueDetachLv2EventQueue),
    HLE_TODO(cellSpursRunJobChain),
    HLE_TODO(cellSpursShutdownJobChain),
    HLE_TODO(cellSpursShutdownTaskset),
    HLE_TODO(cellSpursTaskAttributeSetExitCodeContainer),
    HLE_TODO(cellSpursTaskExitCodeInitialize),
    HLE_TODO(cellSpursTasksetAttributeSetName),
    HLE_TODO(cellSpursTasksetAttributeSetTasksetSize),
});

}  // namespace rt
