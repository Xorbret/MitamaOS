# Writing Shugyo lessons and Harae drills

Content files are Lua files that `return` a table. The files here are built into the firmware (`tools/mkstudy.py` turns them into `src/study/builtin_content.h`). Extra lessons and drills can go on the SD card in `/MitamaOS/study/lessons/` and `/MitamaOS/study/scenarios/`; a file with the same id as a built-in one replaces it. Files are listed in filename order, so prefix them with their track (`t1-02-...`). CI loads every file here and refuses a broken or unfair one.

## Lesson (`lessons/*.lua`)

```lua
return {
  id = "t1-02-i2c-scan",          -- unique; used for saved progress
  track = 1,                      -- 0 Orientation, 1 Build, 2 Connect, 3 Test, 4 Secure
  title = "Finding devices on I2C",
  permissions = { "i2c" },        -- modules the learner's code may use
  pages = { "Page one...", "Page two..." },
  task = "What the learner must do.",
  starter = [[ starter code ]],
  check = function(r)             -- r.ok, r.output, r.error, r.code
    return true, "feedback shown to the learner"
  end,
  quiz = {
    { q = "Question?", choices = { "A", "B", "C" }, answer = 2, why = "Explanation." },  -- answer is 1-based
  },
  reflect = "A help-desk style reflection prompt.",
}
```

`check` runs in its own sandbox with read-only hardware (`imu`, `i2c`, `sys`), so it can compare the learner's output with a live reading. Leave out `check` for a reading-and-quiz lesson.

### Modules available to learner code

| Permission | Lua API |
| --- | --- |
| `display` | `display.clear()`, `display.print(text)` |
| `gpio` | `gpio.mode(pin, "in"/"out")`, `gpio.write(pin, 0/1)`, `gpio.read(pin)` (Grove pins only) |
| `imu` | `imu.read()` returns `ax, ay, az, gx, gy, gz` |
| `i2c` | `i2c.scan()` returns a list of addresses on the Grove port |
| `wifi` | `wifi.scan()` returns `{ ssid, rssi, auth, channel }` entries (passive) |
| `files` | `files.read(path)`, `files.write(path, text)` inside `/MitamaOS/user/Study` |
| `sys` | `sys.millis()`, `sys.sleep(ms)` (up to 5000) |

## Scenario (`scenarios/*.lua`)

```lua
return {
  id = "s01-office-wifi",
  title = "Trouble on the office Wi-Fi",
  faction = "Static",             -- Harae faction (optional)
  demon = "The Severance",        -- the demon behind it (optional)
  briefing = "Set the scene.",
  points = 3,                     -- action points per round
  rounds = {
    {
      symptom = "What the Blue Team sees, without naming the attack.",
      damage = { uptime = 40, data = 0 },   -- applied if the response fails (half if partial)
      evidence = {
        { name = "AP event log", cost = 1, text = "What the evidence shows." },
      },
      defenses = {
        { name = "The right fix", result = "stop", ward = "t4-01-wifi-recon", text = "Why it works." },
        { name = "A half fix", result = "partial", text = "What it misses." },
        { name = "A wrong fix", result = "fail", text = "Why it does nothing." },
      },
      reveal = {
        attack = "Name of the attack",
        target = "What it goes after",
        cia = "Availability",
        technique = "MITRE ATT&CK or OWASP IoT reference",
        lesson = "lesson id to study next",
      },
    },
  },
}
```

Scoring per round: 100 for `stop`, 50 for `partial`, 0 for `fail`, plus 10 per unspent action point when the response helped.

## Fairness and Wards

Every round needs at least one `stop` response, or the file is refused: no drill can be impossible.

A response can name a `ward`: the id of the Shugyo lesson that teaches it. Passing that lesson's task earns the Ward. A drill whose rounds can't all be stopped with the Wards the player holds is shown as sealed, with the lesson to pass, so it is never offered as an unwinnable board. Responses without a `ward` are always open.
