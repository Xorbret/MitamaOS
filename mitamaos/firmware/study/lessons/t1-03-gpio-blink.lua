return {
  id = "t1-03-gpio-blink",
  track = 0,
  title = "Driving a pin",
  permissions = { "gpio", "sys" },
  pages = {
    "GPIO pins are switches the CPU controls: output high (3.3 V) or low (0 V), or input to read a button.",
    "Lessons may only use the Grove port pins (G1 and G2). The pins wired to the screen, keyboard and SD card are blocked by the sandbox.",
    "Wire an LED with a resistor from G2 to GND on a Grove breakout, or a Grove LED unit, to see it blink. Nothing plugged in is fine too: the check reads your code.",
    "How the code works:\n|for i = 1, 5 do ... end\nRepeats the middle part 5 times.\n|gpio.write(2, 1)\nTurns pin 2 on. A 0 turns it off.\n|sys.sleep(200)\nWaits 200 milliseconds.",
  },
  task = "Make pin 2 blink 5 times. The code turns it on but never off. Change the line that starts with --.",
  starter = [[gpio.mode(2, "out")
for i = 1, 5 do
  gpio.write(2, 1)
  sys.sleep(200)
  -- turn it off and wait again
end
print("done")]],
  hint = "Replace the -- line with these two lines:\n|gpio.write(2, 0)\n|sys.sleep(200)",
  check = function(r)
    if not r.ok then return false, r.error end
    local code = r.code:gsub("%-%-[^\n]*", "")
    local offs = select(2, code:gsub("gpio%.write%(%s*2%s*,%s*0", ""))
    if offs == 0 then return false, "Your loop never turns the pin off (gpio.write(2, 0))." end
    if not r.output:find("done", 1, true) then return false, "Print 'done' at the end." end
    return true, "Blinking. Same idea drives relays, buzzers and door strikes."
  end,
  quiz = {
    { q = "Why does the sandbox block the LCD and SD pins?", choices = { "To save power", "Least privilege: code only gets what it needs", "They are input only" },
      answer = 2, why = "Least privilege limits damage from buggy or malicious code." },
  },
  reflect = "Name one real device where a stuck-on GPIO output could be a safety issue.",
}
