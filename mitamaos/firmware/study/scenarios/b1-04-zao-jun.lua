return {
  id = "b1-04-zao-jun",
  title = "Lunch hour",
  faction = "Babel",
  demon = "Zao Jun",
  briefing = "Not every outage is Babel. The Kitchen God keeps getting blamed for Babel's noise, and he's tired of it. Rule out the boring cause before you call it an attack.",
  points = 3,
  surge = 5,
  rounds = {
    {
      symptom = "Break-room Wi-Fi dies every day at about 12:10, and only in the break room. By one o'clock it's fine again.",
      damage = { uptime = 20, data = 0 },
      evidence = {
        { name = "Spectrum read", cost = 2, free_with = "b1-03-interference-or-jamming",
          text = "A wide hump across channels 9 to 11 from 12:08 to 12:50. Every other channel is quiet." },
        { name = "Access point log", cost = 1, text = "No deauthentication frames. Just a lot of retries on channel 10." },
        { name = "Ask the staff", cost = 1, text = "\"It's worst when someone's heating soup.\"" },
      },
      defenses = {
        { name = "Change band: move the break-room access point to 5 GHz or a clear channel", result = "stop",
          text = "The microwave only leaks around 2.4 GHz channels 9 to 11. Move away from it and everyone eats in peace. Maybe give Zao Jun some honey." },
        { name = "Sealed Frames", result = "fail", ward = "t4-01-wifi-recon",
          text = "Nothing was forged. You burned a Seal on a microwave." },
        { name = "Iron Road", result = "partial", ward = "b1-04-wired-backups",
          text = "A cable works for a desk, but nobody wants a cable to their sandwich. A Seal spent on a harmless spirit." },
        { name = "Report the jamming", result = "fail",
          text = "It isn't jamming. It's lunch. The regulator won't thank you." },
      },
      reveal = {
        attack = "Not an attack: microwave interference",
        target = "2.4 GHz channels near a microwave oven",
        cia = "Availability",
        technique = "Troubleshooting: rule out the ordinary cause first",
        lesson = "b1-03-interference-or-jamming",
      },
    },
  },
}
