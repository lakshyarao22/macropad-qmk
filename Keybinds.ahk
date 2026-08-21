#Requires AutoHotkey v2.0

cDown := 0

*CapsLock::
{
    global cDown
    Send "{Blind}{Ctrl Down}"
    cDown := A_TickCount
}


*CapsLock Up::
{
    global cDown
    if (A_TickCount - cDown) < 150
        Send "{Blind}{Ctrl Up}{Esc}"
    else
        Send "{Blind}{Ctrl Up}"
}

PgDn::Send "{Media_Play_Pause}"

; Remap Shift+Backspace → Delete
+Backspace::{
    Send "{Delete}"
}

^#t:: {
    WinSetAlwaysOnTop -1, "A"
}

~MButton & WheelUp:: {
    SoundSetVolume "+5" ; increase by 5 percentage points
}

~MButton & WheelDown:: {
    SoundSetVolume "-5" ; decrease by 5 percentage points
}

#q:: Send "!{F4}"

;Text Substitutions

#SingleInstance Force
SetKeyDelay 20, 20   ; a bit of delay helps in Teams/Outlook

; Trigger includes the trailing space: fires when you type "greet␠"
:*?B0:greet ::
{
    ; 1) Capture the name the user types next, stopping on Space/Enter/Tab
    ih := InputHook("V", "{Space}{Enter}{Tab}")
    ih.Start()
    ih.Wait()
    userName := Trim(ih.Input)
    endKey   := ih.EndKey

    if (userName = "")
        return

    ; 2) If Enter/Tab inserted a newline/tab, eat it so caret sits after the name
    if (endKey = "Enter" || endKey = "Tab")
        SendEvent "{BS}"
    ; If Space ended input, move left one so selection starts right at the end of the name
    else if (endKey = "Space")
        SendEvent "{Left}"

    ; 3) Select the just-typed name AND the preceding "greet " word
    ; Ctrl+Shift+Left twice selects: "<Name>" and then "greet"
    SendEvent "^+{Left 2}"

    ; 4) Overwrite selection with a random greeting
    greetings := [
        "Hello " userName ", hope you are doing well.",
        "Hi " userName ", I trust you’re having a productive day.",
        "Hi " userName ", hope your week is going well.",
        "Hello " userName ", hope you're doing well.",
        "Hi " userName ", I hope everything is going smoothly.",
        "Hello " userName ", great to connect with you.",
        "Hi " userName ", I trust you're doing well."
    ]
    SendText greetings[Random(1, greetings.Length)]
}

F21:: {
    SoundSetMute(-1, , "Microphone")
}


F13::
{
    if WinActive("ahk_exe ms-teams.exe")
    {
        Send("Thanks for the confirmation{Enter}")
    }
	if WinActive("ahk_exe msedge.exe")
	{
		Run("https://es-watchguard.splunkcloud.com/en-GB/app/SplunkEnterpriseSecuritySuite/incident_review?earliest=-24h%40h&latest=now#/")
	}
    if WinActive("ahk_exe explorer.exe")
    {
        Run("https://es-watchguard.splunkcloud.com/en-GB/app/SplunkEnterpriseSecuritySuite/incident_review?earliest=-24h%40h&latest=now#/")
    }
}

F14::
{
    if WinActive("ahk_exe msedge.exe")
	{
		Run("https://watchguard.atlassian.net/jira/your-work")
	}
}
F15::
{
    if WinActive("ahk_exe ms-teams.exe")
    {
        Send("^g")
    }
	if WinActive("ahk_exe msedge.exe")
	{
		Run("https://cloud.watchguard.com")
	}
}
F16::
{
    if WinActive("ahk_exe msedge.exe")
	{
		Run("https://github.com/")
	}
}
F17::
{
    if WinActive("ahk_exe msedge.exe")
	{
		Run("https://usa.authpoint.watchguard.com/watchguard/resources")
	}
}
F18::
{
    if WinActive("ahk_exe msedge.exe")
	{
		Run("https://watchguardtechnologies.zendesk.com/agent/home/tickets")
	}
}