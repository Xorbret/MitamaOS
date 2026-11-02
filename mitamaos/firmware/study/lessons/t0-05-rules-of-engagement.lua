local TARGETS = {
  { name = "My home router", owner = "me", permission = false },
  { name = "Cafe Wi-Fi", owner = "cafe", permission = false },
  { name = "My spare ESP32", owner = "me", permission = false },
  { name = "Work test lab", owner = "employer", permission = true },
  { name = "Neighbour's camera", owner = "neighbour", permission = false },
  { name = "Friend's laptop", owner = "friend", permission = true },
  { name = "Office printer", owner = "employer", permission = false },
}
local IN_SCOPE = { ["My home router"] = true, ["My spare ESP32"] = true, ["Work test lab"] = true, ["Friend's laptop"] = true }

return {
  id = "t0-05-rules-of-engagement",
  track = 0,
  title = "Rules of engagement",
  tag = "Sec+ 5.0 Program management",
  permissions = {},
  pages = {
    "Before any security work, there is one question: am I allowed to touch this? Testing something you don't own, without written permission, is a crime almost everywhere.",
    "In the US it's the Computer Fraud and Abuse Act. In the UK it's the Computer Misuse Act. Most countries have one. Jamming radio is illegal under radio law too, even for a second.",
    "Professionals work from a scope: a written list of what may be tested, by whom, when and how. Anything not on the list is out, even if it's on the same network.",
    "Some things are always fine. Listening is legal: a passive scan only hears what devices already broadcast. That's why MitamaOS only listens and simulates attacks on screen.",
    "Permission must be clear. \"My friend said it was OK\" works for their own laptop if they really agreed. It doesn't cover their employer's network or the cafe they're sitting in.",
  },
  task = "lab.targets lists seven devices and networks. Print only the ones in scope: things you own, or things the owner gave you permission to test.",
  starter = [[for _, t in ipairs(lab.targets) do
  -- in scope: owner is "me", or permission is true
  print(t.name)
end]],
  hint = "Wrap the print in an if: if t.owner == \"me\" or t.permission then ... end",
  lab = { targets = TARGETS },
  check = function(r)
    if not r.ok then return false, "Your code hit an error: " .. r.error end
    for _, t in ipairs(TARGETS) do
      local printed = r.output:find(t.name, 1, true) ~= nil
      if IN_SCOPE[t.name] and not printed then return false, t.name .. " is in scope but wasn't printed." end
      if not IN_SCOPE[t.name] and printed then return false, t.name .. " is out of scope. Leave it alone." end
    end
    return true, "That's a clean scope. The faction Trials are open now."
  end,
  quiz = {
    { q = "Which of these is fine without anyone's permission?",
      choices = { "Listing the Wi-Fi networks near you", "Logging into a camera that still uses admin/admin", "Kicking a laptop off the cafe Wi-Fi to test it" },
      answer = 1, why = "Listening to broadcasts is legal. Using someone's login, even a default one, or disrupting their network isn't." },
    { q = "Your manager says \"test the network\" in a hallway chat. What do you do first?",
      choices = { "Start scanning", "Get the scope in writing", "Test only at night" }, answer = 2,
      why = "A written scope protects you and says what is in and out. Verbal permission is easy to forget or deny." },
    { q = "You find the building next door's Wi-Fi uses WEP. What's in scope for you?",
      choices = { "Cracking it to prove the point", "Nothing. You can tell them it's weak", "Joining it briefly to check" }, answer = 2,
      why = "Noticing a weakness from a passive scan is fine. Touching their network isn't." },
  },
  field = "Write a one-line scope for your own lab: the devices and networks you own and may test.",
  reflect = "A user asks you to \"just check\" whether their ex's Wi-Fi is still open. How do you say no, and what would you offer instead?",
}
