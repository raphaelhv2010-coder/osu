# osu!mania 4K pour NumWorks (.nwa)

Application externe **osu!mania 4K** écrite en C++ natif (API EADK NumWorks) pour calculatrice NumWorks (Epsilon / Omega / Upsilon).

## Contrôles en jeu
- **Colonne 1 (Cyan)** : touche `7`
- **Colonne 2 (Rose)** : touche `8`
- **Colonne 3 (Rose)** : touche `(`
- **Colonne 4 (Cyan)** : touche `)`
- **Menu / Validation** : Flèches `Haut`/`Bas` + `OK` ou `EXE`
- **Quitter la partie / Rejouer** : `BACK` (ou `HOME` pour quitter l'application)

## Fonctionnalités fidèles à osu!mania
- **6 niveaux progressifs** :
  1. `1* Beginner` (100 BPM — rythmes simples + Long Notes)
  2. `2* Easy` (125 BPM — escaliers et maintiens)
  3. `3* Normal` (145 BPM — accords à 2 touches / Jumps)
  4. `4* Hard` (168 BPM — trilles rapides et alternances)
  5. `5* Insane` (190 BPM — streams 16e et mini-jacks)
  6. `6* Expert+` (222 BPM — inspiré de *Freedom Dive*, flux continus et chordjacks)
- **Notes simples & Long Notes (LN)** : maintien requis jusqu'à la fin de la traînée.
- **Système de jugement complet** : `MAX 300!`, `300!`, `200`, `100`, `50`, `MISS`.
- **Barre de vie (HP)** dynamique, compteur de **Combo**, calcul de **Précision (%)** et attribution de **Rang** (`SS`, `S`, `A`, `B`, `C`, `D`, `F`).

## Compiler le fichier `.nwa`
1. **En local (Linux / macOS / WSL)** :
   ```bash
   sudo apt install gcc-arm-none-eabi make
   make
   ```
   Cela génère directement le fichier **`osu_mania_4k.nwa`**.
2. **Via GitHub Actions (sans rien installer)** :
   Poussez ce dossier sur un dépôt GitHub : le workflow `.github/workflows/build.yml` compile automatiquement `osu_mania_4k.nwa` dans l'onglet *Actions*.
3. **Transfert sur la calculatrice** :
   Utilisez l'outil officiel [my.numworks.com/apps](https://my.numworks.com/apps) (ou `nwlink` / Upsilon Web Installer) pour envoyer `osu_mania_4k.nwa` sur votre NumWorks.
