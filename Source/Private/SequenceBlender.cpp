#include "SequenceBlender.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "SequenceBlender"

void FSequenceBlender::StartupModule()
{
}

void FSequenceBlender::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSequenceBlender, SequenceBlender)
DEFINE_LOG_CATEGORY(LogSequenceBlender);
