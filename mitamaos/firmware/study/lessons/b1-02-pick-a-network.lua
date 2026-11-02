local SAVED = { ssid = "Office", auth = "wpa2" }
local SCAN = {
  { ssid = "Office", auth = "wpa2", channel = 6, rssi = -52 },
  { ssid = "Office", auth = "open", channel = 1, rssi = -41 },
  { ssid = "Office-Guest", auth = "wpa2", channel = 6, rssi = -55 },
  { ssid = "Office", auth = "open", channel = 11, rssi = -44 },
  { ssid = "Free Fast WiFi", auth = "open", channel = 3, rssi = -47 },
  { ssid = "Office", auth = "wep", channel = 9, rssi = -46 },
  { ssid = "Cafe Next Door", auth = "wpa2", channel = 1, rssi = -71 },
  { ssid = "0ffice", auth = "open", channel = 4, rssi = -43 },
}
local FAKES = 3   -- exact name, wrong security type

return {
  id = "b1-02-pick-a-network",
  track = 1,
  title = "How devices pick a network",
  tag = "Sec+ 2.0 Threats and vulnerabilities",
  card = "True Name",
  requires = { "t0-05-rules-of-engagement" },
  spirit = "Will-o'-the-wisps",
  taunt = "Foolish fire. It's right there in the name. Follow one light over the marsh and it drifts just out of reach until you're up to your knees in the bog. Your phone sees a hundred networks called \"Office\" and picks the loudest. Ours is open. Come closer.",
  permissions = {},
  pages = {
    "A Wi-Fi network's name (its SSID) is just a label. Anyone can broadcast any name, for free, from a cheap board.",
    "Your phone remembers networks you've joined as saved profiles. Each profile holds the name and the security type: open, WPA2 or WPA3.",
    "A careless device joins anything with the right name. A careful one checks the security type too. An \"Office\" that suddenly has no password isn't your office.",
    "The attack this Trial seals: a beacon flood. One device broadcasts hundreds of fake networks. The real one gets buried, and people tap a fake \"Office\" that's open and run by someone else.",
    "The fix, True Name: devices join saved profiles that check the security type, never a name picked from a long list. Forget open networks you don't need, and use WPA3 where you can.",
  },
  task = "lab.saved is your saved profile and lab.scan is what's in the air. Print every network with exactly the saved name but a different security type. Then print how many you found.",
  starter = [[local fakes = 0
for _, n in ipairs(lab.scan) do
  if n.ssid == lab.saved.ssid then
    -- a fake has the right name but the wrong auth
    print(n.ssid .. " " .. n.auth .. " ch" .. n.channel)
  end
end
print(fakes)]],
  hint = "Add a second test: and n.auth ~= lab.saved.auth. Then add 1 to fakes inside the if.",
  lab = { saved = SAVED, scan = SCAN },
  check = function(r)
    if not r.ok then return false, "Your code hit an error: " .. r.error end
    local last
    for n in r.output:gmatch("%d+") do last = tonumber(n) end
    if last == nil then return false, "Print the number of fakes on the last line." end
    if r.output:find("wpa2 ch6", 1, true) then return false, "The wpa2 Office on channel 6 is the real one. Don't list it." end
    if last ~= FAKES then return false, "The check counts " .. FAKES .. " fakes. You printed " .. last .. "." end
    return true, "Three wisps named. You earned True Name: the Will-o'-the-wisps can be bound."
  end,
  quiz = {
    { q = "The scan also shows \"0ffice\" with a zero, open. Your code didn't count it. Is it safe?",
      choices = { "Yes, the name is different", "No. It's a lookalike, and your phone won't join it by itself, but people might tap it", "Yes, it's on another channel" },
      answer = 2, why = "Devices won't auto-join a different name, but tired people tap lookalikes. Train staff to pick the saved network, not a name from the list." },
    { q = "Which setting best protects a phone from fake copies of a network?",
      choices = { "Auto-join any network with a known name", "Saved profiles that check the security type, and forgetting old open networks", "Turning the screen brightness down" },
      answer = 2, why = "Matching name and security type, and not keeping open networks saved, removes what a fake copy relies on." },
  },
  field = "On your phone, open the list of saved Wi-Fi networks. Forget any open ones you don't use, so it can't auto-join a fake copy.",
  reflect = "A user says the Wi-Fi list at work is full of hundreds of strange names and they can't find the real one. What do you tell them to do right now, and what do you report?",
}
