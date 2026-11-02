local DEVICES = {
  { name = "Security camera", critical = true, moves = false },
  { name = "Door controller", critical = true, moves = false },
  { name = "Card reader at the till", critical = true, moves = false },
  { name = "Staff phone", critical = true, moves = true },
  { name = "Smart speaker", critical = false, moves = false },
  { name = "Hot-desk laptop", critical = true, moves = true },
  { name = "File server", critical = true, moves = false },
  { name = "Guest tablet", critical = false, moves = true },
}
local WIRE = { ["Security camera"] = true, ["Door controller"] = true, ["Card reader at the till"] = true, ["File server"] = true }

return {
  id = "b1-04-wired-backups",
  track = 1,
  title = "Wired backups",
  tag = "Sec+ 3.0 Security architecture",
  card = "Iron Road",
  requires = { "t0-05-rules-of-engagement" },
  spirit = "Nurikabe",
  taunt = "Tap me at the bottom with a stick. Or, you know, run a cable. You can't out-shout me on the air. But I can't stand in a wire.",
  permissions = {},
  pages = {
    "Radio is shared air. Anything that must keep working through noise, interference or a jammer is safer on a cable.",
    "The rule is simple. If a device matters and doesn't move, give it a cable: cameras, door controllers, card readers, servers, the access points' own uplinks.",
    "Things that move, like phones and laptops, stay on Wi-Fi. The plan for them is a fallback: mobile data, a hotspot, or a wired desk to sit at.",
    "The attack this Trial seals: jamming. Nothing you set on the router stops it. You go around it, keep the important things up, and report it. In the US that's the FCC, and in the UK it's Ofcom.",
    "The fix, Iron Road: cables for the things that matter, a fallback for the rest, then report the jamming. Write down when it started and what stopped, so the report is useful.",
  },
  task = "lab.devices lists eight devices. Print the ones that should have a cable: critical and not moving. Then print how many.",
  starter = [[local wired = 0
for _, d in ipairs(lab.devices) do
  -- cable it if it matters and doesn't move
  print(d.name)
end
print(wired)]],
  hint = "Use: if d.critical and not d.moves then ... end, and count inside the if.",
  lab = { devices = DEVICES },
  check = function(r)
    if not r.ok then return false, "Your code hit an error: " .. r.error end
    for _, d in ipairs(DEVICES) do
      local printed = r.output:find(d.name, 1, true) ~= nil
      if WIRE[d.name] and not printed then return false, d.name .. " matters and stays put. Give it a cable." end
      if not WIRE[d.name] and printed then
        return false, d.name .. (d.moves and " moves around. It stays on Wi-Fi." or " isn't critical. It can wait.")
      end
    end
    local last
    for n in r.output:gmatch("%d+") do last = tonumber(n) end
    if last ~= 4 then return false, "Print the count (4) on the last line." end
    return true, "Four on the Iron Road. You earned it: the Nurikabe can be bound."
  end,
  quiz = {
    { q = "A jammer is blocking every Wi-Fi channel. Which setting on the router fixes it?",
      choices = { "WPA3", "A different channel", "None. Use cables and report it" }, answer = 3,
      why = "A jammer drowns every channel, whatever the settings. Cables and a report are the only answer." },
    { q = "Who do you report radio jamming to in the US?",
      choices = { "The FCC", "The ISP", "The router maker" }, answer = 1,
      why = "The FCC enforces radio law in the US. In the UK it's Ofcom. Your ISP can't do anything about the air." },
  },
  field = "At home, list the devices that should have a cable (TV box, desktop, NAS, camera hub). Check which ones actually do.",
  reflect = "Your manager asks why you want to spend money on cables when \"Wi-Fi works fine\". Make the case in two sentences.",
}
