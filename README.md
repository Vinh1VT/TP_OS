# TP OS

## TP1 : 

Contient le code de l'interpréteur "biceps" et de la librairie de gestion des commandes "gescom"


## TP2 :

Contient le code du client et du serveur beuip. 
Utilisable depuis l'interpréteur via les commandes (partie 3 du TP) : 

- `beuip start <pseudo>` pour démarrer le serveur et rejoindre le réseau
- `beuip stop` pour quitter le réseau et arrêter le serveur
- `mess list` pour obtenir la liste des personnes connectées sur le serveur
- `mess send <pseudo> <message>` pour envoyer un message à un utilisateur
- `mess broadcast <message>` pour envoyer un message à tout le réseau 

Le client / serveur est aussi utilisable indépendamment, en décommentant leur main, et en utilisant le makefile présent dans leur dossier.

Le client prend alors les commandes de la manière suivante : 
`./clicreme <code> <_pseudo> <message>` (le pseudo est éventuellement optionel (pour les codes 3,5 et 0)).
Les codes des messages sont ceux précisés dans le TP.