# filesystem

Realizzazione di un semplice filesystem che gestirà i file di testo e di cartella, tramite utilizzo di bitmap per la gestione dello spazio in memoria e un Disk Driver che prevede la creazione del disco che si andrà ad usare e le funzioni per manipolare i blocchi.

Avremo due tipologie di blocchi:

Blocchi di dati contenti informazioni casuali: formati da un primo blocco FirstFileBlock che contiene l’header, il FileControlBlock e le informazioni definite da un'array di char, mentre i blocchi successivi sono formati da un header e un’array di char (data). Un file di testo è quindi caratterizzato da un FirstFileBlock e da eventuali altri blocchi, i FileBlock. File di cartella ossia file contenenti altri file che, così come per i file di testo, sono formati da un primo blocco FirstDirectoryBlock che contiene l’header, il FileControlBlock e i blocchi iniziali dei file (testo o cartella) all'interno della cartella. I blocchi successivi invece sono formati da un header e un array di primi blocchi. Un file di cartella è quindi caratterizzato da un FirstControlBlock e da eventuali altri blocchi, i DirectoryBlock.
