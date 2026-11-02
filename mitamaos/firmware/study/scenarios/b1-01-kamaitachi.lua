return {
  id = "b1-01-kamaitachi",
  title = "The Severance",
  faction = "Babel",
  demon = "Kamaitachi",
  briefing = "A small office, one access point, a dozen laptops and two cameras. Something in the air keeps cutting everyone off. It never stays to watch.",
  points = 3,
  surge = 5,
  rounds = {
    {
      symptom = "Every laptop and camera drops off the Wi-Fi at the same moment, then rejoins a few seconds later. Again. And again.",
      damage = { uptime = 40, data = 0 },
      evidence = {
        { name = "Access point log", cost = 1,
          text = "Hundreds of 'deauthenticated, reason 7' events, all claiming to come from the access point's own address." },
        { name = "Spectrum read", cost = 2, free_with = "b1-03-interference-or-jamming",
          text = "The channel is clean. No noise, no wall. Something is sending valid-looking frames, not drowning the air." },
        { name = "ISP status page", cost = 1, text = "No outages reported." },
      },
      defenses = {
        { name = "Sealed Frames: turn on protected management frames (802.11w) or WPA3", result = "stop", ward = "t4-01-wifi-recon",
          text = "The router signs its goodbyes now, so the forged ones are ignored. The Kamaitachi's sickles pass straight through." },
        { name = "Change band", result = "partial",
          text = "Moving to 5 GHz helps only if the attacker's gear is 2.4 GHz only. The cuts slow down, but the real fix is signed frames." },
        { name = "Change the Wi-Fi password", result = "fail",
          text = "On WPA2 the goodbye frames aren't covered by the password, so this changes nothing." },
        { name = "Reboot the access point", result = "fail",
          text = "The forged frames start again the moment everyone reconnects." },
      },
      reveal = {
        attack = "Deauthentication flood",
        target = "802.11 management frames, which WPA2 doesn't sign by default",
        cia = "Availability",
        technique = "MITRE ATT&CK: Network Denial of Service (T1498)",
        lesson = "t4-01-wifi-recon",
      },
    },
  },
}
