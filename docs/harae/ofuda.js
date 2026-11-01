/* Harae: shared ofuda symbol set and card list.
   Every symbol is drawn at the Cardputer's real pixel size, then scaled. */
(function(){
const BM={
 wind:null,
 hearth:["....#....","...##....","..#..#...","..#...#..",".#..#..#.",".#.##..#.",".#.....#.","..#...#..","...###..."],
 gate:["...###...","..#...#..","..#...#..","..#...#..","...#.#...","...#.#...","..#...#..","..#####..","........."],
 hand:["....#....","...###...","....#....",".........","#.......#","#.......#",".#.....#.","..#####..","........."],
 seal:["#########","#.......#","#.#####.#","#.#...#.#","#.#.#.#.#","#.#...#.#","#.#####.#","#.......#","#########"],
 bless:["..#####..",".#.....#.","#.......#","#...#...#","#..###..#","#...#...#","#.......#",".#.....#.","..#####.."],
 shiki:["...###...","...###...","....#....","#########","...###...","...###...","...#.#...","..#...#..","..#...#.."],
 charm:["....#....","...#.#...","..#...#..",".#..#..#.","#..###..#",".#..#..#.","..#...#..","...#.#...","....#...."],
 temizu:["#.#.#.#.#",".........","#.......#",".........","#.......#",".........","#.......#",".........","#.#.#.#.#"]
};
(function(){const r=[];for(let y=0;y<9;y++)r.push(".........".split(""));const off=[1,0,0,1,2,2,1,0,0];for(const base of [1,5])for(let c=0;c<9;c++)r[base+off[c]][c]="#";BM.wind=r.map(a=>a.join(""))})();
function bm(x,name,X,Y,col,sc){sc=sc||1;x.fillStyle=col;BM[name].forEach((row,j)=>{for(let i=0;i<row.length;i++)if(row[i]==="#")x.fillRect(X+i*sc,Y+j*sc,sc,sc)})}
function grid(n){const g=[];for(let y=0;y<n;y++)g.push(new Array(n).fill(0));const P={g,
 px(x,y){x=Math.round(x);y=Math.round(y);if(x>=0&&y>=0&&x<n&&y<n)g[y][x]=1},
 line(x0,y0,x1,y1){x0=Math.round(x0);y0=Math.round(y0);x1=Math.round(x1);y1=Math.round(y1);let dx=Math.abs(x1-x0),sx=x0<x1?1:-1,dy=-Math.abs(y1-y0),sy=y0<y1?1:-1,e=dx+dy;for(;;){P.px(x0,y0);if(x0===x1&&y0===y1)break;const e2=2*e;if(e2>=dy){e+=dy;x0+=sx}if(e2<=dx){e+=dx;y0+=sy}}},
 arc(cx,cy,r,a0,a1){const st=Math.max(12,r*8);let p0=null;for(let i=0;i<=st;i++){const a=(a0+(a1-a0)*i/st)*Math.PI/180;const p=[cx+r*Math.cos(a),cy+r*Math.sin(a)];if(p0)P.line(p0[0],p0[1],p[0],p[1]);p0=p}},
 rect(x,y,w,h){P.line(x,y,x+w,y);P.line(x+w,y,x+w,y+h);P.line(x+w,y+h,x,y+h);P.line(x,y+h,x,y)}};return P}
const SP={
 kamaitachi(P){P.arc(9,11,8,195,285);P.arc(10,12,6,200,275);P.line(2,12,4,10);P.line(3,13,12,13);P.line(6,14,13,14)},
 wisps(P){const f=(x,y)=>{P.px(x,y);P.line(x-1,y+1,x+1,y+1);P.line(x-1,y+2,x+1,y+2);P.px(x,y+3)};f(3,8);f(7,3);f(11,7);P.px(3,13);P.px(7,9);P.px(11,13)},
 nurikabe(P){P.rect(1,2,12,11);P.line(1,6,13,6);P.line(1,9,13,9);P.line(5,2,5,6);P.line(9,6,9,9);P.line(4,9,4,13);P.line(10,9,10,13)},
 zaojun(P){P.line(2,8,12,8);P.arc(7,8,5,0,180);P.line(4,14,10,14);P.line(5,1,5,3);P.px(6,4);P.line(5,5,5,6);P.line(9,1,9,3);P.px(8,4);P.line(9,5,9,6)},
 gaki(P){P.arc(7,9,5,0,360);P.line(7,1,7,4);P.line(5,1,9,1);P.px(7,9)},
 poltergeist(P){P.rect(1,2,6,11);P.px(5,8);P.arc(6,7,4,-50,50);P.arc(6,7,6,-50,50);P.arc(6,7,8,-45,45)},
 vikhor(P){let p0=null;for(let i=0;i<=60;i++){const a=i*0.32,r=0.6+i*0.105;const p=[7+r*Math.cos(a),7+r*Math.sin(a)];if(p0)P.line(p0[0],p0[1],p[0],p[1]);p0=p}},
 pan(P){[[1,3],[4,5],[7,7],[10,9],[13,11]].forEach(([x,top])=>P.line(x,top,x,13));P.line(0,10,14,10)},
 tower(P){P.rect(5,1,4,3);P.rect(3,4,8,4);P.rect(1,8,12,5);P.line(7,4,8,7);P.line(8,7,6,10)},
 omoikane(P){P.line(7,0,7,14);P.line(0,7,14,7);P.line(4,4,10,10);P.line(10,4,4,10)},
 hitogata(P){P.arc(7,2.5,2,0,360);P.line(1,6,13,6);P.line(1,6,1,8);P.line(13,6,13,8);P.line(1,8,4,8);P.line(10,8,13,8);P.line(4,8,4,14);P.line(10,8,10,14);P.line(4,14,6,14);P.line(8,14,10,14);P.line(6,14,7,11);P.line(8,14,7,11)},
 honey(P){P.line(7,1,7,3);P.arc(7,9,5,-90,270);P.line(5,8,9,8)},
 stick(P){P.line(2,1,11,12);P.line(9,13,14,13);P.line(11,12,13,12)},
 lantern(P){P.line(5,1,9,1);P.rect(4,2,6,9);P.line(5,12,9,12);P.line(7,4,7,8)},
 misogi(P){P.line(4,0,4,9);P.line(7,0,7,10);P.line(10,0,10,9);P.line(1,12,4,11);P.line(4,11,7,13);P.line(7,13,10,11);P.line(10,11,13,12)},
 band(P){P.arc(7,13,4,200,340);P.arc(7,13,8,205,335);P.arc(7,13,12,210,330)},
 torii(P){P.line(0,2,3,3);P.line(3,3,11,3);P.line(11,3,14,2);P.line(2,6,12,6);P.line(4,3,4,14);P.line(10,3,10,14);P.px(7,4);P.px(7,5)},
 orizuru(P){P.line(1,10,7,10);P.line(7,10,11,6);P.line(11,6,12,3);P.line(12,3,14,4);P.line(3,10,6,2);P.line(6,2,8,10);P.line(1,10,3,13)},
 kitsune(P){P.line(2,1,4,6);P.line(2,1,6,4);P.line(12,1,10,6);P.line(12,1,8,4);P.line(6,4,8,4);P.line(4,6,7,13);P.line(10,6,7,13);P.px(5,8);P.px(9,8)},
 sealed(P){P.rect(2,2,10,10);P.line(5,7,9,7);P.px(7,5);P.px(7,9)}
};
function spirit(x,name,X,Y,col,sc){sc=sc||1;const P=grid(15);SP[name](P);x.fillStyle=col;P.g.forEach((row,j)=>row.forEach((v,i)=>{if(v)x.fillRect(X+i*sc,Y+j*sc,sc,sc)}))}
const INK={seal:"#e71429",bless:"#00b8cc",shiki:"#e0407a",charm:"#d9a514",temizu:"#8c7dad"};
const KINDS={
 seal:{name:"Seal",jp:"封",desc:"Binds one spirit. The only way to stop it, and every drill can be won with the Seals you've earned."},
 bless:{name:"Blessing",jp:"加護",desc:"A kami's help with investigating: a free clue, a cheaper read. Never needed to win."},
 shiki:{name:"Shikigami",jp:"式神",desc:"Your partner for a Deep Aether dive. It acts every round and charges one Summon per floor. It's paper, so some spirits tear it."},
 charm:{name:"Charm",jp:"守",desc:"Bends a rule. Worn for a whole drill, or spent once for a big effect. Builds and broken combos live here."},
 temizu:{name:"Temizu",jp:"手水",desc:"The basics everyone holds: everyday cyber hygiene. Fixes harmless spirits and scores modestly."}
};
const SUITS={
 wind:{name:"Wind",jp:"風",desc:"The air: Wi-Fi, Bluetooth, radio."},
 hearth:{name:"Hearth",jp:"竈",desc:"Your network's plumbing: addresses, names, cables."},
 gate:{name:"Gate",jp:"門",desc:"Who and what you trust: names, logins, profiles."},
 hand:{name:"Hand",jp:"手",desc:"Things you touch and carry: USB, cards, phones."}
};
/* The card list. Add new cards here and every page picks them up. */
const CARDS=[
 {id:"frames",no:1,set:"BABEL",name:"Sealed Frames",kind:"seal",suit:"wind",cost:1,sp:"kamaitachi",centre:"Kamaitachi",pantheon:"Japanese",wave:1,
  effect:"Binds the Kamaitachi. Forged \"disconnect\" messages are ignored, so nobody gets cut off.",real:"Protected management frames (802.11w) or WPA3.",earn:"Shugyo lesson: Wi-Fi recon"},
 {id:"truename",no:2,set:"BABEL",name:"True Name",kind:"seal",suit:"gate",cost:1,sp:"wisps",centre:"Will-o'-the-wisps",pantheon:"British and Irish",wave:1,
  effect:"Binds the Will-o'-the-wisps. Also the second move against the Tower.",real:"Devices join a saved profile that checks the security type, never a name picked from a list.",earn:"Shugyo lesson: How devices pick a network"},
 {id:"ironroad",no:3,set:"BABEL",name:"Iron Road",kind:"seal",suit:"hearth",cost:1,sp:"nurikabe",centre:"Nurikabe",pantheon:"Japanese",wave:1,
  effect:"Binds the Nurikabe and halves any outage damage. The third move against the Tower.",real:"Cables for the things that matter, then report the jamming, because it's illegal.",earn:"Shugyo lesson: Wired backups"},
 {id:"eye",no:4,set:"BABEL",name:"Spectrum Eye",kind:"bless",suit:"wind",cost:0,sp:"omoikane",centre:"Omoikane",pantheon:"Japanese",wave:1,
  effect:"Omoikane, kami of wisdom, lends you sight: the spectrum view is free and never triggers Surge.",real:"Reading a spectrum analyser to tell interference from jamming.",earn:"Shugyo lesson: Interference or jamming?"},
 {id:"band",no:5,set:"BABEL",name:"Change Band",kind:"temizu",suit:"wind",cost:1,sp:"band",centre:"",pantheon:"",wave:1,
  effect:"Fixes Zao Jun's microwave trouble. Partly helps against a Kamaitachi stuck on 2.4 GHz.",real:"Move to 5 GHz or a clear channel.",earn:"Held from the start"},
 {id:"honey",no:6,set:"BABEL",name:"Honey for the Stove",kind:"charm",suit:"hearth",cost:0,sp:"honey",centre:"Zao Jun",pantheon:"Chinese",wave:1,
  effect:"Worn. Harmless spirits score double Purity.",real:"Ruling out the boring cause before calling it an attack.",earn:"Deep Aether shrine"},
 {id:"stick",no:7,set:"BABEL",name:"Stick at the Base",kind:"charm",suit:"hearth",cost:0,sp:"stick",centre:"Nurikabe",pantheon:"Japanese",wave:1,
  effect:"Worn. The first move against any wall costs no action points.",real:"Knowing where to look first.",earn:"Deep Aether shrine"},
 {id:"lantern",no:8,set:"BABEL",name:"Foolish Fire Lantern",kind:"charm",suit:"gate",cost:0,sp:"lantern",centre:"Will-o'-the-wisps",pantheon:"British and Irish",wave:1,
  effect:"Worn. The first false clue in each drill is marked for you.",real:"Spotting a decoy before you follow it.",earn:"Deep Aether shrine"},
 {id:"misogi",no:9,set:"BABEL",name:"Misogi",kind:"charm",suit:"hearth",cost:1,sp:"misogi",centre:"Waterfall",pantheon:"Japanese",wave:1,spent:true,
  effect:"Spent. A purifying waterfall: restore 20 uptime once, then the charm is used up.",real:"A planned restart that brings services back fast.",earn:"Deep Aether shrine"},
 {id:"pool",no:10,set:"BABEL",name:"Pool Guard",kind:"seal",suit:"hearth",cost:1,sp:"gaki",centre:"Gaki",pantheon:"Buddhist",wave:2,
  effect:"Binds the Gaki, the hungry ghosts that eat every address.",real:"DHCP snooping and a lease limit per port.",earn:"Shugyo lesson: DHCP and DNS by hand"},
 {id:"quiet",no:11,set:"BABEL",name:"Quiet Pockets",kind:"seal",suit:"hand",cost:1,sp:"poltergeist",centre:"Poltergeist",pantheon:"German",wave:2,
  effect:"Binds the Poltergeist and its endless \"pair this device?\" popups.",real:"Nearby-device popups off, phones updated, Bluetooth off when unused.",earn:"Shugyo lesson: Bluetooth advertising"},
 {id:"gatekeeper",no:12,set:"BABEL",name:"Gatekeeper",kind:"seal",suit:"wind",cost:1,sp:"vikhor",centre:"Vikhor",pantheon:"Slavic",wave:2,
  effect:"Binds the Vikhor, the whirlwind of fake visitors at the router's door.",real:"Client limits, rate limiting, and an alert when joins spike.",earn:"Shugyo lesson: Floods and rate limits"},
 {id:"steady",no:13,set:"BABEL",name:"Steady Hand",kind:"seal",suit:"gate",cost:1,sp:"pan",centre:"Pan",pantheon:"Greek",wave:2,
  effect:"Binds Pan and the panic he spreads with fake \"you've been hacked\" calls.",real:"Check through a channel you already trust. Never call a number from a popup.",earn:"Shugyo lesson: Scareware and fake calls"}
];
/* Shikigami: Deep Aether partners. Pick one per dive. */
CARDS.push(
 {id:"hitogata",no:1,set:"SHIKI",name:"Hitogata",kind:"shiki",suit:"hearth",cost:0,sp:"hitogata",centre:"Paper doll",pantheon:"Japanese",wave:1,
  effect:"Every round: soaks the first Surge hit. Summon, once per floor: takes all the damage of one round.",real:"A spare path that keeps people working while you investigate.",earn:"Break the Tower",style:"Defence"},
 {id:"orizuru",no:2,set:"SHIKI",name:"Orizuru",kind:"shiki",suit:"wind",cost:0,sp:"orizuru",centre:"Paper crane",pantheon:"Japanese",wave:1,
  effect:"Every round: your first clue costs 1 less. Summon, once per floor: every clue on the board is revealed for free.",real:"Good monitoring that tells you where to look first.",earn:"Find it at a Deep Aether shrine",style:"Scouting"},
 {id:"kitsune",no:3,set:"SHIKI",name:"Kitsune",kind:"shiki",suit:"gate",cost:0,sp:"kitsune",centre:"Fox",pantheon:"Japanese",wave:1,
  effect:"Every round: Kindred combos add +0.5 more Resonance. Summon, once per floor: copies the last Seal you played.",real:"Reusing a fix that already worked somewhere else.",earn:"Score Kindred ten times",style:"Combo"}
);
CARDS.forEach(c=>c.of=CARDS.filter(d=>d.set===c.set).length);
// a text-free slip, 29x52 at sc=1
function slipS(x,X,Y,c,sc,sel){sc=sc||1;const w=29*sc,h=52*sc;x.fillStyle=sel?"#fff6d6":"#efe7b5";x.fillRect(X,Y,w,h);
  x.fillStyle=c.kind==="charm"?"#d9a514":"#7b0818";x.fillRect(X,Y,w,sc);x.fillRect(X,Y+h-sc,w,sc);x.fillRect(X,Y,sc,h);x.fillRect(X+w-sc,Y,sc,h);
  if(sel){x.strokeStyle="#ffee08";x.lineWidth=Math.max(1,sc/2);x.strokeRect(X-1.5*sc/2-1,Y-1.5*sc/2-1,w+1.5*sc+2,h+1.5*sc+2)}
  const ink="#1a0f12";bm(x,c.suit,X+3*sc,Y+3*sc,ink,sc);
  for(let i=0;i<c.cost;i++){x.fillStyle=c.kind==="temizu"?ink:INK[c.kind];x.fillRect(X+w-6*sc-i*4*sc,Y+4*sc,3*sc,3*sc)}
  if(c.cost===0){x.fillStyle=ink;x.fillRect(X+w-6*sc,Y+5*sc,3*sc,sc)}
  x.fillStyle=ink;x.fillRect(X+3*sc,Y+14*sc,w-6*sc,sc);
  spirit(x,c.sp,X+7*sc,Y+17*sc,ink,sc);
  x.fillRect(X+3*sc,Y+35*sc,w-6*sc,sc);
  bm(x,c.kind,X+10*sc,Y+39*sc,INK[c.kind],sc)}
function back(x,X,Y,sc){sc=sc||1;const w=29*sc,h=52*sc;x.fillStyle="#210052";x.fillRect(X,Y,w,h);x.fillStyle="#e71429";
  x.fillRect(X+2*sc,Y+2*sc,w-4*sc,sc);x.fillRect(X+2*sc,Y+h-3*sc,w-4*sc,sc);x.fillRect(X+2*sc,Y+2*sc,sc,h-4*sc);x.fillRect(X+w-3*sc,Y+2*sc,sc,h-4*sc);
  spirit(x,"torii",X+7*sc,Y+18*sc,"#ffee08",sc)}
/* draw a pixel thing into a fresh canvas inside el, at scale sc */
function canvasIn(el,w,h,sc,bg){const cv=document.createElement("canvas");cv.width=w*sc;cv.height=h*sc;el.appendChild(cv);const x=cv.getContext("2d");if(bg){x.fillStyle=bg;x.fillRect(0,0,cv.width,cv.height)}return x}
const SETS={BABEL:"Babel",SHIKI:"Shikigami"};
window.Ofuda={SETS,BM,SP,INK,KINDS,SUITS,CARDS,bm,spirit,slipS,back,canvasIn,
  card:id=>CARDS.find(c=>c.id===id)};
})();
