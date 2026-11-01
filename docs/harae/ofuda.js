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
 shio:["....#....","..##.##..","##.....##","#.##.##.#","#...#...#","#...#...#","##..#..##","..##.##..","....#...."],
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
Object.assign(SP,{
 changeling(P){P.arc(4,6,3,0,360);P.arc(10,6,3,0,360);P.line(7,0,7,14);P.arc(7,13,6,200,340);P.px(4,6);P.px(10,6)},
 pixies(P){for(let i=0;i<8;i++){const a=i*Math.PI/4;P.px(7+6*Math.cos(a),7+6*Math.sin(a));P.px(7+6*Math.cos(a)+1,7+6*Math.sin(a))}P.line(7,5,7,9);P.line(5,7,9,7)},
 puca(P){P.line(4,0,5,7);P.line(6,0,6,7);P.line(10,0,9,7);P.line(8,0,8,7);P.arc(7,10,4,0,360);P.px(5,9);P.px(9,9)},
 leanan(P){P.arc(4,5,3,180,360);P.arc(10,5,3,180,360);P.line(1,5,7,13);P.line(13,5,7,13);P.line(7,13,7,14)},
 brownie(P){P.line(7,0,2,12);P.line(7,0,12,12);P.line(2,12,12,12);P.px(5,8);P.px(9,8);P.line(1,14,13,14)},
 nicnevin(P){P.line(1,4,1,10);P.line(13,4,13,10);P.line(1,4,4,8);P.line(4,8,7,2);P.line(7,2,10,8);P.line(10,8,13,4);P.line(1,10,13,10);P.arc(7,13,3,180,360)},
 kelpie(P){P.arc(7,4,3,0,360);P.line(0,10,3,8);P.line(3,8,6,10);P.line(6,10,9,8);P.line(9,8,12,10);P.line(12,10,14,8);P.line(0,13,3,11);P.line(3,11,6,13);P.line(6,13,9,11);P.line(9,11,12,13);P.line(12,13,14,11)},
 fetch(P){P.arc(7,6,5,0,360);P.line(7,11,7,14);P.line(4,14,10,14);P.line(7,2,7,10)},
 feardearg(P){P.line(2,6,12,6);P.line(3,6,7,0);P.line(11,6,7,0);P.arc(7,10,3,0,360);P.px(6,10);P.px(8,10)},
 cradle(P){P.arc(7,6,6,20,160);P.line(1,8,13,8);P.line(7,0,7,6);P.line(5,0,9,0)},
 coat(P){P.line(4,1,10,1);P.line(4,1,1,5);P.line(10,1,13,5);P.line(3,4,3,14);P.line(11,4,11,14);P.line(3,14,11,14);P.line(7,1,7,14)},
 road(P){P.line(5,0,1,14);P.line(9,0,13,14);P.line(7,1,7,3);P.line(7,6,7,8);P.line(7,11,7,13)},
 nothanks(P){P.arc(7,7,6,0,360);P.line(3,3,11,11)},
 bridle(P){P.arc(7,7,5,0,360);P.arc(7,7,3,0,360);P.line(7,0,7,2);P.line(7,12,7,14)},
 mark(P){P.rect(1,3,12,9);P.line(1,3,7,8);P.line(13,3,7,8);P.px(7,1)},
 door(P){P.rect(3,1,8,13);P.px(9,8);P.line(1,14,13,14)},
 inventory(P){P.rect(1,1,12,12);P.line(3,4,11,4);P.line(3,7,11,7);P.line(3,10,8,10)},
 clover(P){P.arc(5,5,2,0,360);P.arc(9,5,2,0,360);P.arc(5,9,2,0,360);P.arc(9,9,2,0,360);P.line(7,9,9,14)},
 rowan(P){P.line(7,0,7,14);[[2,4],[2,8],[12,4],[12,8]].forEach(([x,y])=>P.line(7,y+2,x,y));P.px(6,2);P.px(8,2);P.px(7,1)},
 cream(P){P.line(0,8,14,8);P.arc(7,8,6,0,180);P.line(5,14,9,14);P.px(7,1);P.line(6,2,8,2);P.line(6,3,8,3);P.px(7,4)},
 saltline(P){P.line(0,12,14,12);for(let i=1;i<14;i+=3){P.px(i,10);P.px(i+1,11)}P.rect(4,1,6,7)}
});
Object.assign(SP,{
 rakshasa(P){P.line(2,0,4,4);P.line(12,0,10,4);P.arc(7,8,5,0,360);P.px(5,7);P.px(9,7);P.line(5,11,5,13);P.line(9,11,9,13)},
 vetala(P){P.line(0,1,14,1);P.line(7,1,7,3);P.arc(7,7,3,0,360);P.line(4,6,0,10);P.line(10,6,14,10);P.line(0,10,3,9);P.line(14,10,11,9);P.px(6,8);P.px(8,8)},
 pishacha(P){const pts=[];for(let i=0;i<10;i++){const a=i*Math.PI/5;P.px(7+3*Math.cos(a),3+3*Math.sin(a))}for(let y=7;y<=14;y+=2){P.px(4,y);P.px(10,y)}for(let x=0;x<=14;x+=2)P.px(x,8)},
 maricha(P){P.arc(7,11,3,0,360);P.line(7,8,7,6);P.line(6,8,2,2);P.line(8,8,12,2);P.line(3,4,1,4);P.line(11,4,13,4);P.line(4,2,4,0);P.line(10,2,10,0);P.px(6,11);P.px(8,11)},
 yaksha(P){P.arc(7,10,5,0,180);P.line(2,10,12,10);P.line(4,6,10,6);P.line(4,6,2,10);P.line(10,6,12,10);P.px(5,3);P.px(9,3);P.px(7,1);P.px(7,4)},
 ravana(P){P.arc(7,10,3,0,360);P.px(6,10);P.px(8,10);P.line(4,6,10,6);P.px(4,5);P.px(7,5);P.px(10,5);[[0,9],[1,5],[3,1],[6,0],[9,1],[12,5],[13,9]].forEach(([x,y])=>{P.px(x,y);P.px(x+1,y);P.px(x,y+1);P.px(x+1,y+1)});P.line(5,14,9,14)},
 rahu(P){P.arc(7,7,6,0,360);P.arc(11,7,5,110,250);P.px(6,4);P.px(5,7);P.px(6,10)},
 kumbhakarna(P){P.arc(6,7,5,20,160);P.px(2,13);P.px(6,14);P.px(10,13);P.line(9,0,13,0);P.line(13,0,9,4);P.line(9,4,13,4)},
 indrajit(P){P.arc(4,5,3,180,360);P.arc(8,4,3,180,360);P.arc(11,5,3,180,360);P.line(1,7,14,7);P.line(8,8,6,11);P.line(6,11,9,11);P.line(9,11,7,14)},
 rekha(P){for(let i=0;i<16;i++){if(i%2)continue;const a=i*Math.PI/8;P.px(7+6.5*Math.cos(a),8+6*Math.sin(a))}P.line(4,7,7,4);P.line(7,4,10,7);P.rect(5,7,4,4)},
 ring(P){P.arc(7,9,4,0,360);P.line(5,4,9,4);P.line(5,4,7,1);P.line(9,4,7,1)},
 diya(P){P.arc(7,10,5,0,180);P.line(2,10,12,10);P.line(11,10,13,8);P.px(7,3);P.px(7,4);P.line(6,5,8,5);P.line(6,6,8,6);P.px(7,7)},
 sanjeevani(P){P.line(0,14,6,4);P.line(6,4,9,8);P.line(9,8,11,6);P.line(11,6,14,14);P.line(0,14,14,14);P.px(6,2);P.px(5,1);P.px(7,1)}
});
Object.assign(SP,{
 azazel(P){P.line(5,4,2,0);P.line(2,0,0,2);P.line(9,4,12,0);P.line(12,0,14,2);P.line(3,5,11,5);P.line(3,5,5,11);P.line(11,5,9,11);P.line(5,11,9,11);P.px(5,7);P.px(9,7);P.line(6,12,7,14);P.line(8,12,7,14);P.line(1,6,3,6);P.line(11,6,13,6)},
 ornias(P){P.arc(7,5,5,0,180);P.line(2,5,12,5);P.line(4,6,5,9);P.line(10,6,9,9);P.line(8,9,6,12);P.line(6,12,8,12);P.line(8,12,6,14)},
 iblis(P){P.line(7,0,4,6);P.line(7,0,10,6);P.line(4,6,2,10);P.line(10,6,12,10);P.arc(7,10,5,0,180);P.line(7,6,6,10);P.line(6,10,8,12)},
 lilith(P){P.arc(7,8,6,0,360);P.arc(5,7,1.5,0,360);P.arc(9,7,1.5,0,360);P.line(7,9,7,10);P.line(2,1,4,3);P.line(12,1,10,3)},
 dumah(P){P.arc(7,7,6,0,360);P.line(3,5,5,5);P.line(9,5,11,5);P.line(4,10,10,10);P.line(7,6,7,13)},
 legion(P){for(let y=1;y<14;y+=3)for(let x=1;x<14;x+=3){P.px(x,y);P.px(x+1,y)}},
 mammon(P){for(let i=0;i<4;i++){P.line(3,12-i*3,11,12-i*3);P.line(3,13-i*3,11,13-i*3)}P.line(1,14,13,14)},
 mastema(P){P.arc(7,7,6,0,360);P.line(7,7,7,1);P.line(7,7,11,3);P.line(8,2,10,3);P.line(8,3,9,3)},
 resheph(P){[[1,13,9,5],[4,13,12,5],[7,13,14,7]].forEach(([a,b,c,d])=>{P.line(a,b,c,d);P.line(c,d,c-3,d);P.line(c,d,c,d+3)})},
 asmodeus(P){P.arc(4,7,3,90,450);P.arc(10,7,3,-270,90);P.arc(4,7,1,0,360);P.arc(10,7,1,0,360);P.line(4,10,10,10);P.line(7,10,7,14)},
 leviathan(P){P.arc(4,4,3,180,360);P.arc(7,4,3,0,180);P.arc(10,10,3,180,360);P.arc(7,10,3,0,180);P.line(13,10,14,9);P.px(1,3);P.line(1,4,2,5)},
 harp(P){P.arc(7,6,6,0,180);P.line(1,6,1,1);P.line(13,6,13,1);P.line(0,1,14,1);P.line(5,1,5,10);P.line(7,1,7,12);P.line(9,1,9,10);P.line(5,13,9,13)},
 cruse(P){P.arc(7,9,5,0,180);P.line(2,9,12,9);P.line(5,5,9,5);P.line(5,5,4,9);P.line(9,5,10,9);P.px(6,2);P.px(8,1);P.px(7,3)},
 lintel(P){P.rect(3,2,8,12);P.line(1,2,13,2);P.line(1,1,13,1);P.px(7,0);P.px(2,6);P.px(12,6)}
});
function spirit(x,name,X,Y,col,sc){sc=sc||1;const P=grid(15);SP[name](P);x.fillStyle=col;P.g.forEach((row,j)=>row.forEach((v,i)=>{if(v)x.fillRect(X+i*sc,Y+j*sc,sc,sc)}))}
const INK={seal:"#e71429",bless:"#00b8cc",shiki:"#e0407a",charm:"#d9a514",shio:"#ff6d00",temizu:"#8c7dad"};
const KINDS={
 seal:{name:"Seal",jp:"封",desc:"Binds one spirit. The only way to stop it, and every drill can be won with the Seals you've earned."},
 bless:{name:"Blessing",jp:"加護",desc:"A kami's help with investigating: a free clue, a cheaper read. Never needed to win."},
 shiki:{name:"Shikigami",jp:"式神",desc:"Your partner for a Deep Aether dive. It acts every round and charges one Summon per floor. It's paper, so some spirits tear it."},
 charm:{name:"Charm",jp:"守",desc:"Worn in a charm slot and bends a rule for as long as it stays there. Builds and broken combos live here."},
 shio:{name:"Shio",jp:"塩",desc:"Purifying salt. One big effect, thrown once, then it's gone. Your emergency button."},
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
 {id:"misogi",no:9,set:"BABEL",name:"Misogi",kind:"shio",suit:"hearth",cost:1,sp:"misogi",centre:"Waterfall",pantheon:"Japanese",wave:1,spent:true,
  effect:"A purifying waterfall: restore 20 uptime, once.",real:"A planned restart that brings services back fast.",earn:"Deep Aether shrine"},
 {id:"pool",no:10,set:"BABEL",name:"Pool Guard",kind:"seal",suit:"hearth",cost:1,sp:"gaki",centre:"Gaki",pantheon:"Buddhist",wave:2,
  effect:"Binds the Gaki, the hungry ghosts that eat every address.",real:"DHCP snooping and a lease limit per port.",earn:"Shugyo lesson: DHCP and DNS by hand"},
 {id:"quiet",no:11,set:"BABEL",name:"Quiet Pockets",kind:"seal",suit:"hand",cost:1,sp:"poltergeist",centre:"Poltergeist",pantheon:"German",wave:2,
  effect:"Binds the Poltergeist and its endless \"pair this device?\" popups.",real:"Nearby-device popups off, phones updated, Bluetooth off when unused.",earn:"Shugyo lesson: Bluetooth advertising"},
 {id:"gatekeeper",no:12,set:"BABEL",name:"Gatekeeper",kind:"seal",suit:"wind",cost:1,sp:"vikhor",centre:"Vikhor",pantheon:"Slavic",wave:2,
  effect:"Binds the Vikhor, the whirlwind of fake visitors at the router's door.",real:"Client limits, rate limiting, and an alert when joins spike.",earn:"Shugyo lesson: Floods and rate limits"},
 {id:"steady",no:13,set:"BABEL",name:"Steady Hand",kind:"seal",suit:"gate",cost:1,sp:"pan",centre:"Pan",pantheon:"Greek",wave:2,
  effect:"Binds Pan and the panic he spreads with fake \"you've been hacked\" calls.",real:"Check through a channel you already trust. Never call a number from a popup.",earn:"Shugyo lesson: Scareware and fake calls"}
];
/* Faction 2: the Good Neighbours */
CARDS.push(
{id:"ironcradle",no:1,set:"GENTRY",name:"Iron in the Cradle",kind:"seal",suit:"wind",cost:1,sp:"changeling",centre:"Changeling",pantheon:"Irish and Scottish",wave:1,
  effect:"Binds the Changeling. Iron: strips one glamour when you play it.",real:"802.1X with server certificate checks, or WPA3, so a copy can't prove it's the real network.",earn:"Shugyo lesson: Enterprise Wi-Fi and certificates"},
{id:"turnedcoat",no:2,set:"GENTRY",name:"Turned Coat",kind:"seal",suit:"gate",cost:1,sp:"pixies",centre:"Pixies",pantheon:"Cornish",wave:1,
  effect:"Binds the Pixies and their fake sign-in pages.",real:"Never type work passwords into a Wi-Fi portal. A password manager that only fills on the real address, and a VPN on public networks.",earn:"Shugyo lesson: Captive portals and fake sign-ins"},
{id:"signedroad",no:3,set:"GENTRY",name:"Signed Road",kind:"seal",suit:"hearth",cost:1,sp:"puca",centre:"Púca",pantheon:"Irish",wave:1,
  effect:"Binds the Púca and its false directions.",real:"DHCP snooping plus a trusted, encrypted DNS resolver, and HTTPS everywhere.",earn:"Shugyo lesson: DNS you can trust"},
{id:"nothanks",no:4,set:"GENTRY",name:"No Thanks",kind:"seal",suit:"gate",cost:1,sp:"leanan",centre:"Leanan Sídhe",pantheon:"Irish",wave:1,
  effect:"Binds the Leanan Sídhe and her endless polite requests.",real:"Number-matching sign-in prompts, never approve one you didn't start, and review which apps can reach your account.",earn:"Shugyo lesson: Sign-in prompts and app consent"},
{id:"bridle",no:5,set:"GENTRY",name:"Silver Bridle",kind:"seal",suit:"hand",cost:1,sp:"kelpie",centre:"Kelpie",pantheon:"Scottish",wave:2,
  effect:"Binds the Kelpie, which rides your signed-in session.",real:"HTTPS everywhere, secure cookies, short sessions, and signing out on shared networks.",earn:"Shugyo lesson: Sessions and cookies"},
{id:"marktrue",no:6,set:"GENTRY",name:"Mark of the True",kind:"seal",suit:"gate",cost:1,sp:"fetch",centre:"Fetch",pantheon:"Irish",wave:2,
  effect:"Binds the Fetch, the double who writes as your boss.",real:"Check the real sender, SPF and DMARC on your own domain, and confirm money requests another way.",earn:"Shugyo lesson: Spotting lookalike senders"},
{id:"closeddoor",no:7,set:"GENTRY",name:"Closed Door",kind:"seal",suit:"hand",cost:1,sp:"feardearg",centre:"Fear Dearg",pantheon:"Irish",wave:2,
  effect:"Binds Fear Dearg, the polite caller who wants your keyboard.",real:"Never give remote access to a call you didn't start. Ring IT back on a number you already know.",earn:"Shugyo lesson: Remote-access scams"},
{id:"knowhouse",no:8,set:"GENTRY",name:"Know Your House",kind:"temizu",suit:"hearth",cost:1,sp:"inventory",centre:"",pantheon:"",wave:1,
  effect:"Clears the Brownie's false alarm.",real:"Keep a list of your own devices, so a stranger stands out and a friend doesn't.",earn:"Shugyo lesson: Keeping a device list"},
{id:"clover",no:9,set:"GENTRY",name:"Four-leaf Clover",kind:"charm",suit:"wind",cost:0,sp:"clover",centre:"Four-leaf clover",pantheon:"Irish",wave:1,
  effect:"Worn. It lets you see through glamour: the first glamoured clue each drill is marked.",real:"Knowing which clue deserves a second look.",earn:"Deep Aether shrine"},
{id:"rowan",no:10,set:"GENTRY",name:"Rowan Sprig",kind:"charm",suit:"gate",cost:0,sp:"rowan",centre:"Rowan",pantheon:"Scottish",wave:1,
  effect:"Worn. Rowan wards off the fae: every Bargain's price is halved.",real:"Healthy suspicion of free gifts.",earn:"Break Nicnevin"},
{id:"saltline",no:11,set:"GENTRY",name:"Salt on the Threshold",kind:"shio",suit:"hearth",cost:1,sp:"saltline",centre:"Salt",pantheon:"Celtic",wave:1,
  effect:"A line of salt the fae can't cross: no glamour for the rest of this round.",real:"Locking things down while you work out what's real.",earn:"Deep Aether shrine"},
{id:"cream",no:12,set:"GENTRY",name:"Cream for the Brownie",kind:"charm",suit:"hearth",cost:0,sp:"cream",centre:"Cream",pantheon:"Scottish",wave:1,
  effect:"Worn. After a harmless round, start the next with 1 extra point.",real:"Not panicking over a friendly device.",earn:"Clear the Brownie without wasting a Seal"}
);
/* Faction 3: the Night-Roamers */
CARDS.push(
{id:"whisper",no:1,set:"LANKA",name:"Whisper",kind:"seal",suit:"wind",cost:1,sp:"rakshasa",centre:"Rakshasas",pantheon:"Ramayana",wave:1,
  effect:"Binds the Rakshasas. Encryption: stops this round's Leak.",real:"HTTPS only, WPA3 or a VPN on shared networks, and retiring apps that talk in the clear.",earn:"Shugyo lesson: Encryption in transit"},
{id:"emptyshoulde",no:2,set:"LANKA",name:"Empty Shoulder",kind:"seal",suit:"hand",cost:1,sp:"vetala",centre:"Vetala",pantheon:"Vetala Panchavimshati",wave:1,
  effect:"Binds the Vetala.",real:"Check installed apps, profiles and device admins, and use a strong lock screen.",earn:"Shugyo lesson: Finding stalkerware"},
{id:"nothingfollo",no:3,set:"LANKA",name:"Nothing Follows",kind:"seal",suit:"hand",cost:1,sp:"pishacha",centre:"Pishachas",pantheon:"Puranas",wave:1,
  effect:"Binds the Pishachas.",real:"Scan for unknown trackers and keep app location and contact permissions minimal.",earn:"Shugyo lesson: Trackers and app permissions"},
{id:"letthedeergo",no:4,set:"LANKA",name:"Let the Deer Go",kind:"seal",suit:"hand",cost:1,sp:"maricha",centre:"Maricha",pantheon:"Ramayana",wave:1,
  effect:"Binds Maricha. Also ends the Lure for this drill.",real:"Never plug in a found drive, check where a QR code goes, install only from trusted sources.",earn:"Shugyo lesson: Bait, USB drops and QR codes"},
{id:"sudarshana",no:5,set:"LANKA",name:"Sudarshana",kind:"seal",suit:"gate",cost:1,sp:"rahu",centre:"Rahu",pantheon:"Puranas",wave:2,
  effect:"Binds Rahu.",real:"A different password everywhere, a password manager, and changing a password the day it leaks.",earn:"Shugyo lesson: Password reuse and breaches"},
{id:"longsleep",no:6,set:"LANKA",name:"Long Sleep",kind:"seal",suit:"hearth",cost:1,sp:"kumbhakarna",centre:"Kumbhakarna",pantheon:"Ramayana",wave:2,
  effect:"Binds Kumbhakarna.",real:"Retire what you don't use, remove old accounts, and update or replace old devices.",earn:"Shugyo lesson: Old devices and forgotten accounts"},
{id:"closedsky",no:7,set:"LANKA",name:"Closed Sky",kind:"seal",suit:"hearth",cost:1,sp:"indrajit",centre:"Indrajit",pantheon:"Ramayana",wave:2,
  effect:"Binds Indrajit.",real:"Check what you've shared, expire old links, and keep cloud storage private by default.",earn:"Shugyo lesson: Cloud sharing and public links"},
{id:"countthetrea",no:8,set:"LANKA",name:"Count the Treasury",kind:"temizu",suit:"hearth",cost:1,sp:"yaksha",centre:"Yakshas",pantheon:"Hindu",wave:1,
  effect:"Clears the Yakshas' false alarm.",real:"Know what your devices back up, where it goes, and when.",earn:"Shugyo lesson: Knowing your backups"},
{id:"lakshmanasli",no:9,set:"LANKA",name:"Lakshmana's Line",kind:"charm",suit:"gate",cost:0,sp:"rekha",centre:"Lakshmana's line",pantheon:"Ramayana",wave:1,
  effect:"Worn. In later tellings Lakshmana drew a line round the hut that nothing evil could cross. The Lure can't pull you out of a round.",real:"Staying inside the boundary when something shiny calls you out.",earn:"Deep Aether shrine"},
{id:"ramasring",no:10,set:"LANKA",name:"Rama's Ring",kind:"charm",suit:"gate",cost:0,sp:"ring",centre:"Rama's ring",pantheon:"Ramayana",wave:1,
  effect:"Worn. Hanuman carried Rama's ring so Sita would know he was a true messenger. Once per drill, verify a clue for free.",real:"Proof of who a message is really from.",earn:"Break Ravana"},
{id:"lampatdusk",no:11,set:"LANKA",name:"Lamp at Dusk",kind:"shio",suit:"hearth",cost:1,sp:"diya",centre:"Diya lamp",pantheon:"Hindu",wave:1,
  effect:"Light a lamp at sunset and the night-walkers keep away: no Leak this round.",real:"Locking down while you investigate.",earn:"Deep Aether shrine"},
{id:"sanjeevani",no:12,set:"LANKA",name:"Sanjeevani",kind:"shio",suit:"hearth",cost:1,sp:"sanjeevani",centre:"Sanjeevani herb",pantheon:"Ramayana",wave:1,
  effect:"The herb Hanuman flew a whole mountain back for: restore all leaked data, once.",real:"A tested restore from backup.",earn:"Deep Aether shrine"}
);
/* Faction 4: the Fallen */
CARDS.push(
{id:"thescapegoat",no:1,set:"FALLEN",name:"The Scapegoat",kind:"seal",suit:"hand",cost:1,sp:"azazel",centre:"Azazel",pantheon:"1 Enoch",wave:1,
  effect:"Binds Azazel. Also the second move against Legion.",real:"Only install signed software from real sources, and quarantine or sandbox anything suspicious.",earn:"Shugyo lesson: Where your software comes from"},
{id:"solomonsring",no:2,set:"FALLEN",name:"Solomon's Ring",kind:"seal",suit:"hearth",cost:1,sp:"ornias",centre:"Ornias",pantheon:"Testament of Solomon",wave:1,
  effect:"Binds Ornias. Also the first move against Legion.",real:"Watch for processes using power they shouldn't, block miners, remove what you can't account for.",earn:"Shugyo lesson: Spotting what's using your machine"},
{id:"knowyourplac",no:3,set:"FALLEN",name:"Know Your Place",kind:"seal",suit:"gate",cost:1,sp:"iblis",centre:"Iblis",pantheon:"Quran",wave:1,
  effect:"Binds Iblis.",real:"Least privilege: everyday accounts aren't admins and apps get only the rights they need.",earn:"Shugyo lesson: Least privilege"},
{id:"threenames",no:4,set:"FALLEN",name:"Three Names",kind:"seal",suit:"hand",cost:1,sp:"lilith",centre:"Lilith",pantheon:"Alphabet of Ben Sira",wave:1,
  effect:"Binds Lilith.",real:"Change default passwords first, update before first use, buy from sellers you trust.",earn:"Shugyo lesson: Setting up a new device safely"},
{id:"cleanslate",no:5,set:"FALLEN",name:"Clean Slate",kind:"seal",suit:"hearth",cost:1,sp:"legion",centre:"Legion",pantheon:"Gospel of Mark",wave:1,
  effect:"Binds Legion, the last move. Wipe and rebuild from a known-good image.",real:"Wipe and rebuild from a known-good image, then restore data from clean backups.",earn:"Shugyo lesson: Rebuilding a compromised device"},
{id:"theark",no:6,set:"FALLEN",name:"The Ark",kind:"seal",suit:"hearth",cost:1,sp:"mammon",centre:"Mammon",pantheon:"Gospel of Matthew",wave:2,
  effect:"Binds Mammon.",real:"Offline, tested backups a flood can't reach. Patch, and never pay.",earn:"Shugyo lesson: Backups that survive ransomware"},
{id:"jubilee",no:7,set:"FALLEN",name:"Jubilee",kind:"seal",suit:"gate",cost:1,sp:"mastema",centre:"Mastema",pantheon:"Jubilees",wave:2,
  effect:"Binds Mastema. Ends 'a tenth remain' for the drill.",real:"Rebuild from clean, rotate every password and key, remove accounts you didn't create.",earn:"Shugyo lesson: Cleaning up after a breach"},
{id:"hedge",no:8,set:"FALLEN",name:"Hedge",kind:"seal",suit:"hearth",cost:1,sp:"resheph",centre:"Resheph",pantheon:"Ugaritic",wave:2,
  effect:"Binds Resheph.",real:"Patch quickly, segment the network, and close services nobody needs.",earn:"Shugyo lesson: Patching and segmentation"},
{id:"tobiasssmoke",no:9,set:"FALLEN",name:"Tobias's Smoke",kind:"seal",suit:"gate",cost:1,sp:"asmodeus",centre:"Asmodeus",pantheon:"Tobit",wave:2,
  effect:"Binds Asmodeus.",real:"Strong sign-in protection, saved recovery codes, alerts when recovery details change.",earn:"Shugyo lesson: Account recovery"},
{id:"thehook",no:10,set:"FALLEN",name:"The Hook",kind:"seal",suit:"hand",cost:1,sp:"leviathan",centre:"Leviathan",pantheon:"Job",wave:2,
  effect:"Binds Leviathan.",real:"Secure boot, verified firmware, and reflashing when you can't trust the layers above.",earn:"Shugyo lesson: Secure boot and firmware"},
{id:"checkthequar",no:11,set:"FALLEN",name:"Check the Quarantine",kind:"temizu",suit:"hand",cost:1,sp:"dumah",centre:"Dumah",pantheon:"Talmud",wave:1,
  effect:"Clears Dumah's false alarm, and sweeps up a remnant.",real:"Look in your antivirus quarantine before assuming files were destroyed.",earn:"Shugyo lesson: What your antivirus does"},
{id:"davidsharp",no:12,set:"FALLEN",name:"David's Harp",kind:"charm",suit:"hearth",cost:0,sp:"harp",centre:"David's harp",pantheon:"1 Samuel",wave:1,
  effect:"Worn. When David played, the evil spirit left Saul (1 Samuel 16:23). Each round, one possessed card is freed.",real:"Steady cleanup, one device at a time.",earn:"Deep Aether shrine"},
{id:"saltofelisha",no:13,set:"FALLEN",name:"Salt of Elisha",kind:"shio",suit:"hearth",cost:1,sp:"cruse",centre:"Elisha's bowl",pantheon:"2 Kings",wave:1,
  effect:"Elisha threw salt from a new bowl into the bad spring and healed it (2 Kings 2). Free every possessed card, once.",real:"A full clean sweep of everything at once.",earn:"Deep Aether shrine"},
{id:"bloodontheli",no:14,set:"FALLEN",name:"Blood on the Lintel",kind:"shio",suit:"gate",cost:1,sp:"lintel",centre:"The lintel",pantheon:"Exodus",wave:1,
  effect:"Mark the doorposts and the destroyer passes over (Exodus 12). No spirit acts this round.",real:"Isolating the network while the danger passes.",earn:"Break Legion"}
);
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
const SETS={BABEL:"Babel",GENTRY:"Good Neighbours",LANKA:"Night-Roamers",FALLEN:"The Fallen",SHIKI:"Shikigami"};
window.Ofuda={SETS,BM,SP,INK,KINDS,SUITS,CARDS,bm,spirit,slipS,back,canvasIn,
  card:id=>CARDS.find(c=>c.id===id)};
})();
