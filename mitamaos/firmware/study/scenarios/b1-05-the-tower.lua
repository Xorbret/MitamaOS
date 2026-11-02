return {
  id = "b1-05-the-tower",
  title = "The Tower",
  faction = "Babel",
  demon = "The Tower",
  briefing = "Everyone's shouting and nobody's saying anything. Classic Babel. The Nurikabe walls off 2.4 GHz, and as everyone flees to 5 GHz, the wisps are waiting with fake copies of the office network. Three moves: find the wall, name the true network, go around.",
  points = 3,
  surge = 5,
  rounds = {
    {
      symptom = "Everything on 2.4 GHz is dead. Phones that can are hopping to 5 GHz on their own. The rest just spin.",
      damage = { uptime = 25, data = 0 },
      evidence = {
        { name = "Spectrum read", cost = 2, free_with = "b1-03-interference-or-jamming",
          text = "2.4 GHz is buried on every channel. 5 GHz is clear of noise but crowded with beacons." },
        { name = "Access point log", cost = 1, text = "No deauthentication frames. Clients can't hear the 2.4 GHz radio at all." },
      },
      defenses = {
        { name = "Find the wall: log the jamming and move everyone who can to 5 GHz", result = "stop",
          text = "You've found the wall and stepped around it for now. The noise underneath is easier to hear." },
        { name = "Reboot the 2.4 GHz radios", result = "partial", text = "They come back into the same noise. You lose time." },
        { name = "Change the Wi-Fi password", result = "fail", text = "A wall doesn't care about passwords." },
      },
      reveal = {
        attack = "Radio jamming on 2.4 GHz",
        target = "The shared air",
        cia = "Availability",
        technique = "Jamming is illegal under radio law",
        lesson = "b1-03-interference-or-jamming",
      },
    },
    {
      symptom = "On 5 GHz there are forty networks called \"Office\". Staff are tapping the strongest one and getting a sign-in page.",
      damage = { uptime = 30, data = 20 },
      evidence = {
        { name = "Nearby networks", cost = 1,
          text = "Forty \"Office\" networks on 5 GHz. Thirty-nine are open. One is WPA3, the real one." },
        { name = "Staff interview", cost = 1, text = "\"It said Office. Of course I tapped it.\"" },
      },
      defenses = {
        { name = "True Name: join only the saved profile that checks the security type", result = "stop", ward = "b1-02-pick-a-network",
          text = "Thirty-nine lights and one true name. Devices go to the real one and the wisps lose their audience." },
        { name = "Pick the strongest \"Office\"", result = "fail", text = "The strongest is always the closest fake." },
        { name = "Hide the network name", result = "fail", text = "Hiding the name doesn't stop anyone copying it." },
      },
      reveal = {
        attack = "Beacon flood on the fallback band",
        target = "People and devices picking a network by name",
        cia = "Availability",
        technique = "MITRE ATT&CK: Network Denial of Service (T1498)",
        lesson = "b1-02-pick-a-network",
      },
    },
    {
      symptom = "The wall spreads to 5 GHz. Now nothing wireless works anywhere in the building.",
      damage = { uptime = 35, data = 0 },
      evidence = {
        { name = "Spectrum read", cost = 2, free_with = "b1-03-interference-or-jamming",
          text = "Both bands buried, every channel, wall to wall." },
        { name = "Device list", cost = 1, text = "The till, cameras, door controller and server never move." },
      },
      defenses = {
        { name = "Iron Road: cable what matters, fall back to mobile data, report the jamming", result = "stop", ward = "b1-04-wired-backups",
          text = "The Tower can drown the air, but it can't stand in a wire. The building runs, the report goes in, and the rabble scatters." },
        { name = "Change band", result = "fail", text = "There's no band left to change to." },
        { name = "Wait it out", result = "partial", text = "Babel gets bored eventually. You lose the day." },
      },
      reveal = {
        attack = "Wide-band jamming",
        target = "Every wireless link in the building",
        cia = "Availability",
        technique = "Resilience: wired paths and reporting",
        lesson = "b1-04-wired-backups",
      },
    },
  },
}
