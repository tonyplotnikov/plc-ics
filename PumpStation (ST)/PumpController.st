FUNCTION_BLOCK PumpController
VAR_INPUT
    StartCmd    : BOOL;
    StopCmd     : BOOL;
    DryRun      : BOOL;
    Overheat    : BOOL;
    AutoMode    : BOOL;
    MinLevel    : BOOL;
    MaxLevel    : BOOL;
END_VAR

VAR_OUTPUT
    PumpOn      : BOOL;
    Alarm       : BOOL;
END_VAR

VAR
    DelayStart  : TON;
END_VAR


// Таймер на 5 секунд задержки запуска
DelayStart(IN := StartCmd AND NOT PumpOn, PT := T#5s);

IF StopCmd THEN
    PumpOn := FALSE;
    Alarm := FALSE;

ELSIF Overheat OR DryRun THEN
    Alarm := TRUE;
    PumpOn := FALSE;

ELSIF AutoMode THEN
    IF NOT MaxLevel AND MinLevel AND DelayStart.Q THEN
        PumpOn := TRUE;
    ELSE
        PumpOn := FALSE;
    END_IF;

ELSE // Ручной режим
    IF StartCmd AND NOT Overheat AND NOT DryRun THEN
        PumpOn := TRUE;
    END_IF;
END_IF;
