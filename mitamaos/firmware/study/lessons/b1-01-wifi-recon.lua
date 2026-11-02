return {
  id = "t4-01-wifi-recon",
  track = 1,
  title = "Wi-Fi recon",
  tag = "Sec+ 4.0 Security operations",
  card = "Sealed Frames",
  requires = { "t0-05-rules-of-engagement" },
  spirit = "Kamaitachi",
  taunt = "Snip, snip. Rude. Weasels on a whirlwind, sickles for claws: the cut is so fast you only find the wound later. Every laptop drops at once, then rejoins. I don't even need your password. Your router believes any \"goodbye\" that claims to be from it. Seal the frames and see how far I get.",
  permissions = { "wifi" },
  pages = {
    "Recon is the first step for attackers and defenders alike: what is out there and how is it protected?",
    "A passive scan only listens for beacons that access points broadcast anyway. It is legal and is what every phone does. Connecting to or attacking networks you do not own is not.",
    "Security types, weakest first: open, WEP (broken), WPA/WPA2-PSK (depends on the password), WPA3 (strongest). Open or WEP on a home or office network is a finding.",
    "The attack this Trial seals: a deauthentication flood. Wi-Fi has small \"management frames\" that say hello and goodbye. On WPA2 they aren't signed, so anyone can forge a goodbye from your router and every device drops at once.",
    "The fix is Protected Management Frames (802.11w, often called PMF). The router signs those frames and devices ignore forged ones. WPA3 requires PMF, which is one more reason to move to it.",
  },
  task = "Scan and print each network as: ssid, auth. Then print how many are open or WEP.",
  starter = [[local nets = wifi.scan()
local weak = 0
for _, n in ipairs(nets) do
  print(n.ssid .. ", " .. n.auth)
  -- count weak ones here
end
print(weak)]],
  check = function(r)
    if not r.ok then return false, r.error end
    if not r.code:find("wifi.scan", 1, true) then return false, "Use wifi.scan()." end
    local last
    for n in r.output:gmatch("%d+") do last = tonumber(n) end
    if last == nil then return false, "Print the weak count on the last line." end
    return true, "Recon done. You earned Sealed Frames: in Harae, the Kamaitachi can be bound."
  end,
  quiz = {
    { q = "Every device drops off the Wi-Fi at once and rejoins seconds later, over and over. Changing the password does nothing. What fixes it?",
      choices = { "A longer password", "Protected Management Frames (802.11w)", "Rebooting the router" }, answer = 2,
      why = "The forged goodbye frames aren't covered by the password on WPA2. PMF signs them, so fakes are ignored." },
    { q = "Which of these is OK without permission?", choices = { "Passively listing nearby networks", "Guessing a neighbor's Wi-Fi password", "Kicking devices off a cafe network" },
      answer = 1, why = "Listening to public beacons is fine. Accessing or disrupting networks you do not own is illegal in most places." },
  },
  field = "On your own router, find the setting called PMF, 802.11w or Protected Management Frames and set it to Capable or Required. WPA3 turns it on for you.",
  reflect = "Your office guest Wi-Fi shows as 'open'. Write the ticket you would raise and who it goes to.",
}
