local READINGS = {
  { label = "12:10 break room", busy = { 9, 10, 11 } },
  { label = "09:00 floor 2", busy = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 } },
  { label = "14:30 lobby", busy = { 1, 2, 3, 4, 5 } },
  { label = "16:45 car park", busy = { 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13 } },
  { label = "18:00 office", busy = { 6 } },
}
local ANSWER = {
  ["12:10 break room"] = "interference", ["09:00 floor 2"] = "jamming", ["14:30 lobby"] = "interference",
  ["16:45 car park"] = "jamming", ["18:00 office"] = "interference",
}

return {
  id = "b1-03-interference-or-jamming",
  track = 1,
  title = "Interference or jamming?",
  tag = "Sec+ 4.0 Security operations",
  card = "Spectrum Eye",
  requires = { "t0-05-rules-of-engagement" },
  spirit = "Nurikabe",
  taunt = "An invisible wall on a night road. Pushing does nothing and going round takes forever. I don't trick anyone. I just stand here, across every channel, and I'm very good at standing here. Is it me, or is it someone's lunch? Better learn the difference.",
  permissions = {},
  pages = {
    "2.4 GHz Wi-Fi has 13 channels (11 in the US). A spectrum view shows how much radio noise sits on each one.",
    "Interference is ordinary noise. A microwave oven leaks a wide hump around channels 9 to 11 while it runs. Baby monitors, old cordless phones and Bluetooth crowd a few channels. It comes and goes, and it's local.",
    "Jamming is deliberate. A jammer buries every channel at once, all day, and no setting turns it off. It's illegal in most countries, including for the person who \"just wanted quiet\".",
    "A simple rule of thumb: if nearly every channel (10 or more of 13) is buried at the same moment, suspect jamming. If a few neighbouring channels are busy, look for an appliance first.",
    "Rule out the boring cause before you call it an attack. Most \"the Wi-Fi is cursed\" tickets are a microwave, a bad spot, or a full channel.",
  },
  task = "lab.readings holds five spectrum reads. For each one, print its label, then jamming if 10 or more channels are busy, or interference if fewer are.",
  starter = [[for _, r in ipairs(lab.readings) do
  local count = #r.busy
  local verdict = "interference"
  -- decide when it's jamming
  print(r.label .. " " .. verdict)
end]],
  hint = "Add: if count >= 10 then verdict = \"jamming\" end",
  lab = { readings = READINGS },
  check = function(r)
    if not r.ok then return false, "Your code hit an error: " .. r.error end
    for label, want in pairs(ANSWER) do
      local line = r.output:match(label:gsub("%p", "%%%0") .. "[^\n]*")
      if not line then return false, "No line for " .. label .. "." end
      local got = line:match("(%a+)%s*$")
      if got ~= want then return false, label .. " should be " .. want .. ". Count its busy channels again." end
    end
    return true, "Two walls, three ordinary noises. You earned Spectrum Eye: in Harae the spectrum read is free."
  end,
  quiz = {
    { q = "Break-room Wi-Fi dies every day at 12:10 and nowhere else. Channels 9 to 11 light up. Most likely?",
      choices = { "A jammer", "The microwave", "A deauth attack" }, answer = 2,
      why = "Same time, same place, a wide hump on 9 to 11: that's a microwave oven running. Move the access point to another channel or to 5 GHz." },
    { q = "Every channel is buried, all day, on the whole floor. What do you do?",
      choices = { "Change the channel", "Switch critical devices to cables and report the jamming", "Change the Wi-Fi password" },
      answer = 2, why = "No channel escapes a jammer. Keep important things running on cables, then report it, because jamming is illegal." },
  },
  field = "At lunchtime, stand near a microwave with Scry or a Wi-Fi analyser app open and watch channels 9 to 11 while it runs.",
  reflect = "A user insists someone is \"hacking the Wi-Fi\" because it drops every day at lunch. How do you explain what's really happening without making them feel silly?",
}
