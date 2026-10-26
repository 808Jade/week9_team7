#include "EnginePCH.h"
#include "EngineLog.h"

namespace
{
    TArray<ILogSink*>& GetSinks()
    {
        static TArray<ILogSink*> Sinks;
        return Sinks;
    }
}

void FLog::AddSink(ILogSink* Sink)
{
    if (Sink)
        GetSinks().Add(Sink);
}

void FLog::RemoveSink(ILogSink* Sink)
{
    TArray<ILogSink*>& Sinks = GetSinks();
    for (int32 Index = Sinks.Num() - 1; Index >= 0; --Index)
        if (Sinks[Index] == Sink)
            Sinks.RemoveAt(Index, 1);
}

void FLog::Emit(ELogVerbosity Verbosity, const FString& Message)
{
    FString DebugMessage = Message;
    if (DebugMessage.empty() || DebugMessage.back() != '\n')
    {
        DebugMessage += "\n";
    }
    OutputDebugStringA(DebugMessage.c_str());

    for (ILogSink* Sink : GetSinks())
        Sink->OnLog(Verbosity, Message);
}
