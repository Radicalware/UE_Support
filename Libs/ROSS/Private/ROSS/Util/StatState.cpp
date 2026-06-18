
#include "ROSS/Util/StatState.h"

FStatState::FStatState()
    {
        if(SvStatNames.Num() == 0){
            SvStatNames.Add(TEXT("TestLatest"));
            SvStatNames.Add(TEXT("TestScore"));
        }
    }