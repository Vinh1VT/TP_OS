NOM: VO TUAN
PRENOM: VINH

# Interpréteur BICEPS - TP OS user

## Structure du code
- **tp1/** : Contient le code de l'interpréteur de commandes `biceps` et la librairie `gescom` pour l'analyse et l'exécution (commandes internes et externes).
- **tp2/** : Contient le code de la premiere version du serveur beuip (UDP).
- **tp3/** : Contient la logique réseau (UDP et TCP) et la gestion des threads pour le chat et l'échange de fichiers (`servtp3`).

## Fonctionnalités 
Ce projet implémente un interpréteur de commandes shell supportant un système de chat local basé sur le protocole BEUIP.
- Multithreading via `pthread` (séparation client de saisie / serveur d'écoute).
- Gestion de liste chaînée dynamique pour les utilisateurs du réseau.
- (Bonus) Échange de fichiers pair-à-pair via TCP.

## Commandes BEUIP :
- `beuip start <pseudo>` : Connexion au réseau
- `beuip list` : Affichage des utilisateurs
- `beuip message <pseudo> <message>` : Message privé
- `beuip message all <message>` : Message à tous
- `beuip stop` : Déconnexion du réseau