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
- Edge-addition algorithm:
- Linear time:

### Important terms
- DFS tree:
- bicomp:
- pertinent:
- externally active:
- walkup:
- walkdown:
- merge/flip:

### Algorithm phases
1.
2.
3.

### Structures needed in code
- ...
- ... 

---

## Library API observations

### Functions exposed
- isPlanar:
- planarEmbed:
- certificate / Kuratowski output:

### Design ideas for my project
- ...