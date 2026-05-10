# Reading Notes

## Boost Boyer-Myrvold docs

### Expected API

- Input je neusmjeren graf sa stabilnim ID-jevima vrhova i grana.
- Ako je graf planaran, algoritam vraća `planar = true` i planar embedding.
- Ako graf nije planaran, algoritam vraća `planar = false` i skup grana koje čine Kuratowski subgraph.
- Za moj projekat, rezultat algoritma treba biti strukturisan kroz:
  - `PlanarityResult`
  - `PlanarEmbedding`
  - `KuratowskiCertificate`

### Embedding format

- Planar embedding nije crtež u koordinatama.
- Planar embedding je kombinatorni opis redoslijeda grana oko svakog vrha.
- U Boost pristupu, embedding se konceptualno može posmatrati kao mapiranje:
  - vertex -> sequence of incident edges
- Ta sekvenca predstavlja redoslijed grana oko tog vrha.
- Za moju implementaciju najjednostavniji format je:
  - `std::vector<std::vector<int>> clockwiseEdgesAroundVertex`
- Svaki unutrašnji `vector<int>` čuva ID-jeve grana u cikličnom redoslijedu oko jednog vrha.
- Koordinate za crtanje se generišu kasnije, u posebnom layout/drawing koraku.

### Rotation system

- Rotation system je ciklični redoslijed incidentnih grana oko svakog vrha.
- To je praktično ista informacija koju čuva planar embedding.
- Rotation system je dovoljan za:
  - provjeru embeddinga,
  - obilazak lica grafa,
  - kasnije generisanje planar drawinga.

### Pomoćne funkcije za drawing

Boost dokumentacija navodi da mnogi algoritmi za crtanje planarnog grafa pretpostavljaju da je graf:

- povezan,
- biconnected,
- maximal planar.

Zbog toga postoje pomoćne funkcije:

- `make_connected` — dodaje minimalan skup grana da graf postane povezan.
- `make_biconnected_planar` — dodaje grane da povezan planaran graf postane biconnected, uz očuvanje planarnosti.
- `make_maximal_planar` — dodaje grane da biconnected planaran graf postane maksimalno planaran.

Za moj projekat ovo znači:

- Boyer-Myrvold prvo treba vratiti embedding.
- Drawing/layout algoritam može imati dodatne pretpostavke.
- Treba jasno odvojiti:
  - planarity testing,
  - embedding,
  - drawing coordinates.

### Verification

Boost navodi dvije važne provjere:

- `is_kuratowski_subgraph`
  - provjerava da li output za neplanaran graf zaista može biti kontrahovan u Kuratowski subgraph.
- `is_straight_line_drawing`
  - provjerava da li nacrtani straight-line drawing nema presijecanja grana.

Za moju implementaciju ovo znači:

- Trebam imati nezavisan `KuratowskiVerifier`.
- Svaki certifikat koji algoritam vrati mora se automatski provjeriti.
- Kasnije, za vizualizaciju, mogu dodati i validator za crtež bez presjeka.

### Certificate

- Za neplanaran graf algoritam treba vratiti skup originalnih edge ID-jeva.
- Taj skup grana treba formirati subdivision od `K5` ili `K3,3`.
- Certifikat ne smije biti samo “neki konflikt” iz algoritma.
- Certifikat mora biti moguće nezavisno validirati.
- Planirani output:
  - `KuratowskiCertificate.type`
  - `KuratowskiCertificate.edgeIds`
  - `KuratowskiCertificate.branchVertices`

### Complexity notes

- Ciljana složenost za planarity algoritme je linearna.
- Za linearno vrijeme nije dovoljno samo koristiti DFS; treba paziti i na pomoćne operacije.
- Ako algoritam mora sortirati, comparison sort može uvesti `O(n log n)`, pa se za striktno linearno vrijeme koristi bucket sort.
- Kod embeddinga su važne operacije nad listama grana oko vrhova.
- Posebno treba paziti na operacije:
  - concatenation,
  - reversal.
- Običan `std::list::reverse` može narušiti linearnost ako se poziva mnogo puta.
- Boost zato koristi specijalizovanu lazy strukturu za mješovite sekvence concatenation/reversal operacija.
- Za moju implementaciju moram jasno dokumentovati:
  - gdje koristim jednostavnije STL strukture,
  - da li to utiče na teorijsku složenost,
  - šta bi trebalo zamijeniti za strogu linearnu implementaciju.

### Implementation conclusions

- `PlanarEmbedding` u mom kodu treba biti odvojen od `Graph` strukture.
- Grane moraju imati stabilne originalne ID-jeve.
- Embedding treba čuvati redoslijed edge ID-jeva oko svakog vrha.
- Drawing treba biti poseban modul nakon embeddinga.
- Kuratowski certifikat treba biti provjeren nezavisnim verifierom.
- U dokumentaciji treba posebno objasniti razliku između:
  - embeddinga,
  - koordinatnog crteža,
  - Kuratowski certifikata.

---

## Boyer-Myrvold paper

### Main idea

- Algorithm type: edge-addition algorithm.
  - (Potpora iz rada: algoritam dodaje grane ulaznog grafa u embedding strukturu `~G`.)

- Algoritam postepeno gradi embedding strukturu `~G`.
  - `~G` ne predstavlja samo konačni embedding, nego i trenutno stanje biconnected komponenti koje nastaju tokom dodavanja grana.
  - (Potpora iz rada: `~G` održava skup BCC-ova koji se razvijaju kako se grane dodaju.)

- Zove se edge-addition zato što se grane grafa dodaju jedna po jedna u postojeću embedding strukturu.
  - Prvo se dodaju DFS tree edgeovi.
  - Zatim se dodaju back edgeovi od trenutnog vrha `v` prema njegovim potomcima.
  - (Potpora iz rada: za svaki vrh `v`, prvo se embeduje tree edge prema DFS djeci, a zatim se obrađuju back edgeovi prema potomcima.)

- Prilikom dodavanja nove grane može doći do spajanja više biconnected komponenti u jednu veću komponentu.
  - (Potpora iz rada: dodavanje nove grane može spojiti dva ili više BCC-ova u jedan veći BCC.)

- Glavno ograničenje pri spajanju komponenti je da vrhovi koji će kasnije učestvovati u dodavanju novih grana moraju ostati na vanjskoj strani / external face-u.
  - Ako komponenta nije u odgovarajućoj orijentaciji, mora se flipati prije merge operacije.
  - (Potpora iz rada: vrh koji će biti potreban za buduće edge embeddinge mora ostati na external face-u; zbog toga BCC ponekad mora biti flipped.)

### Input assumptions

- Graph type: undirected graph.
  - (Potpora iz rada: ulaz je simple undirected graph.)

- Simple graph / multigraph: za osnovnu implementaciju pretpostavljam simple graph.
  - (Potpora iz rada: pseudokod navodi input kao simple undirected graph.)

- Connected / disconnected: algoritam ne zahtijeva da graf bude povezan ili biconnected.
  - To znači da implementacija treba moći obraditi više connected/biconnected komponenti, ili barem jasno definisati kako se to radi u preprocessing fazi.
  - (Potpora iz rada: ulazni graf ne mora biti biconnected niti povezan.)

- DFS assumptions:
  - Algoritam počinje DFS obilaskom.
  - Tokom DFS-a se računaju DFS tree edgeovi, back edgeovi, least ancestor i lowpoint vrijednosti.
  - (Potpora iz rada: preprocessing identifikuje DFS tree edgeove i back edgeove i računa least ancestor i lowpoint vrijednosti.)

### Output

- If planar:
  - Algoritam vraća `PLANAR` i embedding u strukturi `~G`.
  - Nakon što su svi tree edgeovi i back edgeovi uspješno embedovani, planar embedding se recoveruje iz `~G`.
  - (Potpora iz rada: ako su sve grane embedovane, recoveruje se planar embedding.)

- If non-planar:
  - Algoritam vraća `NONPLANAR` i Kuratowski subgraph.
  - Non-planarity se detektuje kada Walkdown ne uspije embedovati sve back edgeove od trenutnog vrha prema njegovim potomcima.
  - (Potpora iz rada: Walkdown fail znači neplanaran graf i tada se poziva izolacija Kuratowski subgrafa.)

- What represents the embedding:
  - Embedding je sadržan u embedding strukturi `~G`.
  - U mojoj implementaciji konačni output treba biti `PlanarEmbedding`, tj. rotation system:
    - `std::vector<std::vector<int>> clockwiseEdgesAroundVertex`
  - (Potpora iz rada: rezultat za planaran graf je embedding u `~G`.)

- What represents the certificate:
  - Certifikat treba biti skup originalnih edge ID-jeva koji formiraju subdivision od `K5` ili `K3,3`.
  - U mojoj implementaciji to treba biti `KuratowskiCertificate`.
  - (Potpora iz rada: za neplanaran graf vraća se Kuratowski subgraph.)

### Important terms

#### DFS tree

- Meaning:
  - DFS tree je stablo nastalo tokom DFS obilaska grafa.
  - Tree edgeovi su grane koje pripadaju DFS stablu.
  - Back edgeovi su netree grane koje povezuju vrh s njegovim DFS pretkom ili potomkom.

- Needed in code as:
  - `parent[v]`
  - `dfsIndex[v]`
  - `children[v]`
  - lista tree edgeova
  - lista back edgeova
  - lowpoint / least ancestor vrijednosti

- (Potpora iz rada: preprocessing identifikuje DFS tree edgeove i back edgeove i računa lowpoint vrijednosti.)

#### Back edge

- Meaning:
  - Back edge je grana koja nije tree edge i koja se u algoritmu dodaje nakon tree edgeova.
  - Kada se obrađuje vrh `v`, algoritam dodaje back edgeove od `v` prema njegovim DFS potomcima.

- Needed in code as:
  - lista back edgeova po vrhu,
  - posebno lista back edgeova koji su incidentni sa trenutno obrađivanim vrhom `v` i vode prema descendant vrhu `w`.

- (Potpora iz rada: za svaki back edge incidentan sa `v` i descendantom `w`, poziva se Walkup, a kasnije Walkdown.)

#### Bicomp

- Meaning:
  - Bicomp je biconnected component u embedding strukturi.
  - Tokom algoritma, svaki tree edge prvo formira malu biconnected komponentu.
  - Kasnije se više bicompova može spojiti u jedan veći bicomp.

- Needed in code as:
  - struktura koja predstavlja biconnected komponentu,
  - root bicomp-a,
  - external face informacije,
  - veze između child bicompova i cut vertexa.

- (Potpora iz rada: tree edge `(v^c, c)` se embeduje kao biconnected component; dodavanje edgeova može spojiti više BCC-ova.)

#### Pertinent

- Meaning:
  - Pertinent znači relevantan za trenutno dodavanje back edgeova prema vrhu `v`.
  - Pertinent subgraph je dio embedding strukture koji će biti uključen u merge operacije zbog dodavanja novih back edgeova prema `v`.

- Needed in code as:
  - `backedgeFlag` za vrhove koji su direktni endpointi back edgeova,
  - `pertinentRoots` lista za cut vertexe,
  - provjera da li je vrh pertinent:
    - ima podignut `backedgeFlag`, ili
    - ima nepraznu `pertinentRoots` listu.

- (Potpora iz rada: pertinent subgraph je skup BCC-ova koji će se spojiti zbog dodavanja novih back edgeova prema `v`.)

#### Externally active

- Meaning:
  - Vrh je externally active ako će biti potreban u budućem dodavanju grana nakon što se završi obrada trenutnog vrha `v`.
  - Takvi vrhovi moraju ostati na external face-u bicompova, jer će kasnije učestvovati u embeddingu novih grana.

- Needed in code as:
  - funkcija ili flag za provjeru external activity,
  - vjerovatno zasnovano na lowpoint vrijednostima,
  - koristi se pri odlučivanju da li se root bicomp-a dodaje na početak ili kraj `pertinentRoots`.

- (Potpora iz rada: externally active vertex će biti uključen u budući embedding edgeova nakon obrade trenutnog `v`.)

#### Internally active

- Meaning:
  - Internally active znači da je vrh ili BCC pertinent, ali nije externally active.
  - Drugim riječima, potreban je sada za trenutni `v`, ali nema vezu prema budućim višim/ranijim dijelovima obrade.

- Needed in code as:
  - stanje izvedeno iz:
    - pertinent,
    - externally active.
  - Ako je pertinent, ali nije externally active, onda je internally active.

- (Potpora iz rada: vertices/BCCs su internally active ako su pertinent, ali nisu externally active.)

#### Inactive

- Meaning:
  - Vrh ili BCC je inactive ako nije ni pertinent ni externally active.

- Needed in code as:
  - može biti izvedeno stanje, vjerovatno ne treba poseban flag ako se može izračunati.

- (Potpora iz rada: inactive znači ni externally ni internally active.)

#### Walkup

- Meaning:
  - Walkup se poziva jednom za svaki back edge `(v, w)`.
  - Njegova svrha je da označi pertinent dijelove embedding strukture zbog tog back edgea.
  - Walkup ne embeduje granu; on priprema informacije za Walkdown.

- Needed in code as:
  - funkcija `walkup(v, w)`,
  - postavlja `backedgeFlag[w] = v`,
  - ažurira `pertinentRoots`,
  - koristi `visited` flag da izbjegne ponovno obrađivanje istog puta za više back edgeova istog vrha `v`.

- (Potpora iz rada: Walkup se poziva za svaki back edge i identifikuje pertinent subgraph koji Walkdown kasnije koristi.)

#### Walkdown

- Meaning:
  - Walkdown koristi informacije koje je pripremio Walkup.
  - Dodaje back edgeove od trenutnog vrha `v` prema descendant vrhovima u pertinent subgraphu.
  - Tokom toga merge-a i flip-a BCC-ove po potrebi.
  - Ako Walkdown ne uspije embedovati sve potrebne back edgeove, graf je neplanaran.

- Needed in code as:
  - funkcija `walkdown(v, childRoot)`,
  - koristi `pertinentRoots`,
  - koristi `backedgeFlag`,
  - izvodi merge/flip operacije,
  - ako ne uspije dodati back edge, poziva se certificate extraction.

- (Potpora iz rada: Walkdown dodaje back edgeove, merge-a i flip-a BCC-ove, a neuspjeh znači neplanarnost.)

#### Merge

- Meaning:
  - Merge je operacija spajanja dva ili više BCC-ova u jedan veći BCC.
  - Dešava se kada dodavanje back edgea povezuje više komponenti kroz cut vertexe.

- Needed in code as:
  - operacija nad embedding/BCC strukturom,
  - mora ažurirati external face i root/cut vertex veze.

- (Potpora iz rada: dodavanje nove grane može spojiti više BCC-ova u jedan veći BCC.)

#### Flip

- Meaning:
  - Flip mijenja orijentaciju BCC-a prije merge-a.
  - Potreban je da relevantni vrhovi ostanu na external face-u.
  - Posebno je bitno za externally active vrhove.

- Needed in code as:
  - operacija nad BCC/external face reprezentacijom,
  - idealno treba biti O(1) ili amortizovano O(1), ne fizičko okretanje cijele liste svaki put.

- (Potpora iz rada: BCC se može flipati prije merge-a da bi budući relevantni vrhovi ostali na external face-u.)

### Algorithm phases

1. Preprocessing
   - Pokrenuti DFS.
   - Izračunati DFS parent/children.
   - Razdvojiti tree edgeove i back edgeove.
   - Izračunati lowpoint i least ancestor vrijednosti.
   - Inicijalizovati embedding strukturu `~G`.
   - Kreirati `separatedDFSChildList` za svaki vrh, sortiranu po child lowpoint vrijednosti.

   (Potpora iz rada: prvi koraci pseudokoda su DFS, lowpoint calculations i inicijalizacija `~G` sa `separatedDFSChildList`.)

2. Obrada vrhova u obrnutom DFS redoslijedu
   - Vrhovi se obrađuju od najvećeg DFS indexa prema najmanjem.
   - Za svaki vrh `v`, prvo se embeduju tree edgeovi prema njegovoj DFS djeci.
   - Svaki takav tree edge inicijalno formira svoj BCC.

   (Potpora iz rada: svaki vertex `v` se obrađuje od `n - 1` do `0`; tree edge `(v^c, c)` se embeduje kao BCC.)

3. Walkup faza
   - Za svaki back edge `(v, w)` gdje je `w` descendant od `v`, poziva se `Walkup`.
   - Walkup označava pertinent vrhove i pertinent BCC roots.
   - Rezultat Walkup faze su informacije koje Walkdown koristi za stvarno dodavanje back edgeova.

   (Potpora iz rada: za svaki back edge incidentan sa `v` i descendantom `w`, poziva se `Walkup(~G, v, w)`.)

4. Walkdown faza
   - Za svako DFS dijete `c` od `v`, poziva se `Walkdown(~G, v^c)`.
   - Walkdown pokušava dodati sve relevantne back edgeove.
   - Tokom toga merge-a i flip-a BCC-ove.
   - Externally active vrhovi moraju ostati na external face-u.

   (Potpora iz rada: Walkdown dodaje back edgeove iz pertinent subgraph-a uz merge/flip i čuvanje externally active vrhova na external face-u.)

5. Provjera neuspjeha
   - Nakon Walkdown faze, algoritam provjerava da li su svi back edgeovi od `v` prema descendantima stvarno dodani u `~G`.
   - Ako neki back edge nije dodat, graf je neplanaran.
   - Tada se izoluje Kuratowski subgraph.

   (Potpora iz rada: ako `(v^c, w)` ne pripada `~G`, poziva se `IsolateKuratowskiSubgraph` i vraća `NONPLANAR`.)

6. Recover planar embedding
   - Ako su svi tree edgeovi i back edgeovi uspješno embedovani, iz `~G` se dobija finalni planar embedding.

   (Potpora iz rada: nakon uspješnog embedovanja svih grana, poziva se `RecoverPlanarEmbedding(~G)`.)

### Walkup details

- Walkup se pokreće za back edge `(v, w)`.
- Prvo označi `w` kao direktno pertinent postavljanjem `backedgeFlag`.
- Zatim traži cut vertexe i BCC roots na putu između `v` i `w`.
- Za cut vertex `r`, root BCC-a `r^s` se dodaje u `pertinentRoots[r]`.
- Ako je taj BCC externally active, root se dodaje na kraj liste; inače se dodaje na početak.
- Walkup koristi `visited` flag da izbjegne ponavljanje rada za isti trenutni vrh `v`.

(Potpora iz rada: `backedgeFlag` označava descendant endpoint, `pertinentRoots` označava cut vertices, a `visited` sprječava ponavljanje ranije obrađenih puteva.)

### Data structures mentioned

- `Graph`
  - originalni neusmjereni graf,
  - stabilni vertex IDs i edge IDs.

- DFS metadata
  - `dfsIndex[v]`
  - `parent[v]`
  - `children[v]`
  - `lowpoint[v]`
  - `leastAncestor[v]`

- Edge classification
  - tree edges,
  - back edges,
  - back edges grouped by ancestor/current vertex.

- Embedding structure `~G`
  - održava trenutno embedovane BCC-ove,
  - čuva external face informacije,
  - omogućava merge i flip operacije.

- BCC / Bicomp structure
  - root vertex,
  - child bicomps,
  - external face representation,
  - internally/externally active status.

- `separatedDFSChildList`
  - lista DFS djece za svaki vrh,
  - sortirana po lowpoint vrijednosti djeteta.

- Per-vertex BM fields
  - `backedgeFlag`
  - `pertinentRoots`
  - `visited`

- Output structures
  - `PlanarEmbedding`
  - `KuratowskiCertificate`

### Implementation questions

- Kako tačno predstaviti embedding strukturu `~G`?
- Kako predstaviti BCC tako da merge i flip budu efikasni?
- Da li prvo implementirati jednostavniju verziju sa STL strukturama, pa kasnije optimizovati za strogu linearnost?
- Kako implementirati `GetSuccessorOnExternalFace`?
- Kako detektovati externally active vertex iz lowpoint/least ancestor informacija?
- Kako iz neuspjelog Walkdown-a izdvojiti Kuratowski certifikat?
- Da li certificate extraction implementirati odmah ili prvo napraviti planarity + embedding dio?

### Complexity notes

- Algoritam cilja linearno vrijeme.
- Svaka grana i vrh smiju biti obrađeni samo konstantan ili amortizovano konstantan broj puta.
- Walkup koristi `visited` flag da ne ponavlja iste puteve za više back edgeova istog vrha `v`.
- Walkup paralelno obilazi dvije strane external face-a da ne plati više od kraće relevantne strane.
- Merge i flip operacije moraju biti pažljivo implementirane, jer fizičko okretanje velikih lista može narušiti linearnost.
- Sortiranje `separatedDFSChildList` po lowpoint vrijednosti treba uraditi tako da ne ugrozi ciljanu složenost.
- Za početnu implementaciju mogu koristiti jednostavnije strukture, ali moram dokumentovati gdje to može odstupiti od stroge linearne složenosti.

### Implementation conclusions for my project

- Prvo trebam implementirati DFS/preprocessing:
  - DFS order,
  - parent/children,
  - tree/back edge classification,
  - lowpoint,
  - least ancestor.

- Zatim trebam napraviti osnovne output/verifier strukture:
  - `PlanarEmbedding`,
  - `EmbeddingValidator`,
  - `KuratowskiCertificate`,
  - `KuratowskiVerifier`.

- BM core ne treba početi prije nego što imam jasne strukture za:
  - BCC,
  - external face,
  - pertinent roots,
  - walkup/walkdown.

- Certificate extraction je poseban i težak dio; ne treba ga miješati s prvim korakom implementacije embeddinga.