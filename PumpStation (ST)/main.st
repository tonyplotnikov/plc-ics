PROGRAM MAIN
VAR
    Pump1  : PumpController;
    Pump2  : PumpController;
    Toggle : BOOL := FALSE;
    TimerToggle : TON;
END_VAR

// Переключатель режимов
IF GlobalVars.StartAuto THEN
    GlobalVars.ModeAuto := TRUE;
ELSIF GlobalVars.StartManual THEN
    GlobalVars.ModeAuto := FALSE;
END_IF;

// Таймер переключения насосов каждые 60 секунд
TimerToggle(IN := TRUE, PT := T#60s);
IF TimerToggle.Q THEN
    Toggle := NOT Toggle;
    TimerToggle(IN := FALSE); // Сброс
    TimerToggle(IN := TRUE);  // Переустановка
END_IF;

IF GlobalVars.ModeAuto THEN
    GlobalVars.CurrentModeText := 'Автоматический';
ELSE
    GlobalVars.CurrentModeText := 'Ручной';
END_IF;

// Ручной режим
IF NOT GlobalVars.ModeAuto THEN
    Pump1(
        StartCmd := GlobalVars.ManualPump1,
        StopCmd := GlobalVars.StopButton,
        DryRun := GlobalVars.DryRunSensor,
        Overheat := GlobalVars.OverheatPump1,
        AutoMode := FALSE,
        MinLevel := GlobalVars.LowLevel,
        MaxLevel := GlobalVars.HighLevel
    );

    Pump2(
        StartCmd := GlobalVars.ManualPump2,
        StopCmd := GlobalVars.StopButton,
        DryRun := GlobalVars.DryRunSensor,
        Overheat := GlobalVars.OverheatPump2,
        AutoMode := FALSE,
        MinLevel := GlobalVars.LowLevel,
        MaxLevel := GlobalVars.HighLevel
    );

ELSE
    // Автоматический режим с чередованием
    IF Toggle THEN
        Pump1(
            StartCmd := TRUE,
            StopCmd := GlobalVars.StopButton,
            DryRun := GlobalVars.DryRunSensor,
            Overheat := GlobalVars.OverheatPump1,
            AutoMode := TRUE,
            MinLevel := GlobalVars.LowLevel,
            MaxLevel := GlobalVars.HighLevel
        );

        Pump2(
            StartCmd := FALSE,
            StopCmd := GlobalVars.StopButton,
            DryRun := GlobalVars.DryRunSensor,
            Overheat := GlobalVars.OverheatPump2,
            AutoMode := TRUE,
            MinLevel := GlobalVars.LowLevel,
            MaxLevel := GlobalVars.HighLevel
        );
    ELSE
        Pump1(
            StartCmd := FALSE,
            StopCmd := GlobalVars.StopButton,
            DryRun := GlobalVars.DryRunSensor,
            Overheat := GlobalVars.OverheatPump1,
            AutoMode := TRUE,
            MinLevel := GlobalVars.LowLevel,
            MaxLevel := GlobalVars.HighLevel
        );

        Pump2(
            StartCmd := TRUE,
            StopCmd := GlobalVars.StopButton,
            DryRun := GlobalVars.DryRunSensor,
            Overheat := GlobalVars.OverheatPump2,
            AutoMode := TRUE,
            MinLevel := GlobalVars.LowLevel,
            MaxLevel := GlobalVars.HighLevel
        );
    END_IF;
END_IF;

// Вывод статусов насосов
GlobalVars.Pump1Status := Pump1.PumpOn;
GlobalVars.Pump2Status := Pump2.PumpOn;
