local TICKETS = {
  { id = 1, text = "Printer on floor 2 is offline again" },
  { id = 2, text = "Payroll spreadsheet totals were changed overnight" },
  { id = 3, text = "A customer list was sent to someone outside the company" },
  { id = 4, text = "Website is down for everyone" },
  { id = 5, text = "Someone edited the price list without permission" },
  { id = 6, text = "A stranger took a photo of my screen on the train" },
  { id = 7, text = "The VPN keeps timing out" },
}
local ANSWER = { "A", "I", "C", "A", "I", "C", "A" }

return {
  id = "t0-04-tickets-cia",
  track = 0,
  title = "Tickets and the CIA triad",
  tag = "Sec+ 1.0 General concepts",
  permissions = {},
  pages = {
    "Every security problem hurts one of three things. Confidentiality: the wrong person sees it. Integrity: someone changes it. Availability: nobody can use it.",
    "That's the CIA triad. It isn't the spy agency. Security people use it to sort a problem before they fix it, because each leg has different fixes.",
    "Confidentiality: leaks, shoulder surfing, a file shared with the wrong person. Integrity: edited records, a tampered update, a fake bank detail. Availability: outages, dropped Wi-Fi, a full disk.",
    "Help Desk already does this. \"The website is down\" is availability. \"My password was used from another country\" starts as confidentiality. Tagging tickets this way tells the security team what to look for.",
    "Harae's factions each attack one leg. Babel goes after availability: it breaks things for fun. The others come later.",
  },
  task = "Seven tickets are in lab.tickets. The starter tags them by keyword, but some come out as ?. Add the missing keywords so every ticket gets C, I or A.",
  starter = [[local words = {
  C = { "leak" },
  I = { "changed" },
  A = { "offline", "down" },
}
for _, t in ipairs(lab.tickets) do
  local tag = "?"
  local text = t.text:lower()
  for leg, list in pairs(words) do
    for _, w in ipairs(list) do
      if text:find(w, 1, true) then tag = leg end
    end
  end
  print(t.id .. " " .. tag)
end]],
  hint = "Run it (fn+R) and see which numbers show ?. Read those tickets and add one word from each to the right list.",
  lab = { tickets = TICKETS },
  check = function(r)
    if not r.ok then return false, "Your code hit an error: " .. r.error end
    local out = "\n" .. r.output
    for i, leg in ipairs(ANSWER) do
      local got = out:match("\n" .. i .. " (%S+)")
      if not got then return false, "Ticket " .. i .. " wasn't printed." end
      if got == "?" then return false, "Ticket " .. i .. " is still ?. Read it and add a keyword." end
      if got ~= leg then return false, "Ticket " .. i .. " came out " .. got .. ". Read it again: who is hurt, and how?" end
    end
    return true, "All seven sorted. That's triage: name the leg, then pick the fix."
  end,
  quiz = {
    { q = "A user's files are encrypted and a note demands money. Which leg is hit first?",
      choices = { "Confidentiality", "Integrity", "Availability" }, answer = 3,
      why = "They can't open their files, so availability goes first. Many ransomware gangs also copy the data, which hits confidentiality too." },
    { q = "An invoice's bank details were quietly changed. Which leg?",
      choices = { "Confidentiality", "Integrity", "Availability" }, answer = 2,
      why = "Nothing was hidden or broken. Data was altered, which is integrity." },
    { q = "Why tag a ticket with C, I or A at all?",
      choices = { "It's required by law", "Each leg points to different checks and fixes", "It makes the ticket shorter" }, answer = 2,
      why = "An outage, a leak and a tampered file are investigated differently. The tag tells the next person where to start." },
  },
  field = "Look at your last three tickets at work, or three tech problems at home. Tag each one C, I or A.",
  reflect = "A user says: \"My email is sending messages I didn't write.\" Which leg would you tag it, and what's the first question you'd ask?",
}
