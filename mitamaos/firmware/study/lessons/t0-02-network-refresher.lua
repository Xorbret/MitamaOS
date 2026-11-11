return {
  id = "t0-02-network-refresher",
  track = 0,
  title = "Network refresher",
  permissions = {},
  pages = {
    "Every IoT device on Wi-Fi needs the same things a laptop does: an IP address (usually from DHCP), a gateway, and DNS to turn names into addresses.",
    "Devices then talk on ports. Common IoT ones: 80 HTTP, 443 HTTPS, 1883 MQTT, 8883 MQTT over TLS, 23 Telnet (old and unsafe).",
    "Help Desk tip: when a device 'can't connect', check in order: Wi-Fi joined, IP assigned, gateway reachable, DNS working, then the service port.",
    "How the code works:\n|local checks = { \"wifi\", \"ip\" }\nMakes a list of words named checks.\n|for i, c in ipairs(checks) do\nRepeats the lines below it once per word. Each time round, c holds the next word.",
    "|print(c)\nShows c on the screen.\n|end\nMarks where the repeated part stops.\nYour job: put print(c) inside the loop, so each word gets shown.",
  },
  task = "Make the loop show each of the four checks on its own line. Change the line that starts with --.",
  starter = [[local checks = { "wifi", "ip", "gateway", "dns" }
for i, c in ipairs(checks) do
  -- replace this line with: print(c)
end]],
  hint = "Replace the -- line with:\n|print(c)",
  check = function(r)
    if not r.ok then return false, r.error end
    local order = { "wifi", "ip", "gateway", "dns" }
    local pos = 0
    for _, word in ipairs(order) do
      local at = r.output:lower():find(word, pos + 1, true)
      if not at then return false, "Missing or out of order: " .. word end
      pos = at
    end
    return true, "Solid habit. The smart-home drill in Harae is open."
  end,
  quiz = {
    { q = "Which port is plain (unencrypted) MQTT?", choices = { "443", "1883", "8883" }, answer = 2,
      why = "1883 is plain MQTT; 8883 is MQTT over TLS." },
    { q = "Which service turns a name into an IP?", choices = { "DHCP", "DNS", "NTP" }, answer = 2,
      why = "DNS resolves names. DHCP hands out addresses. NTP sets the clock." },
  },
  reflect = "Write the first two questions you would ask a user whose smart camera went offline.",
}
