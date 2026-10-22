# Porting del bootloader FoE STM32F407 su F28P65x

Stato: implementazione sorgente e test host; build CCS, salto C28, reset,
retention RAM, ECC e prove di alimentazione sulla scheda ancora da eseguire.
Non è un firmware già collaudato o pronto da caricare in produzione.

## Revisioni analizzate

| Repository | Commit |
| --- | --- |
| f28p65x-soes | `b69e2318108cd072f6b4ddc24c6a2053f10ab2b0` |
| SOES fissato da F28P65x | `1fafe513eba8e07c5c5f1ea2aa4d430518ecf9cf` |
| uc-test fissato da F28P65x | `4605a23a62634d0afec380572a690ce4a1d9d03e` |
| bl-stm32f407 | `9a1a4a8dd2527a7bc0328a5a21fea924e8521456` |
| bl-stm32f4xx-ecat fissato da F407 | `72b497933ed5bf552dcd8e53800ebccb9d6bc295` |
| SOES fissato da F407 | `c0ab53242b7893ffab8b8216b573dc9dc4ed58b8` |

Il commit F28P65x `e82658c5353ee280bdb121b281f04ff4b5b17a8b` introduce
`foe_utils.c` e fissa le due dipendenze sopra. Il file e i riferimenti ai
submodule sono ancora gli stessi nella revisione F28P65x analizzata.
Nessuna PR già aperta risultava nel repository F28P65x durante l'ispezione.

## Flusso STM32 verificato

`Core/Src/main.c` inizializza HAL, clock, CRC e periferiche, poi chiama
`user_code_init()` del submodule `bl-stm32f4xx-ecat`.

1. `user_code_init()` calcola il CRC dell'intera regione applicativa esclusi
   gli ultimi quattro byte e legge il CRC salvato in quei quattro byte.
2. Inizializza SOES e prova ad avviare l'applicazione se i CRC coincidono e
   non è richiesta la permanenza nel bootloader. Nella configurazione senza
   boot pin, la richiesta usa il valore `0xB007B007` nella RAM ESC a `0xF80`.
3. `jump2app()` imposta VTOR, carica MSP e salta al reset handler ARM.
4. Restando nel bootloader, FoE apre il file, cancella la regione applicativa,
   scrive i blocchi e alla chiusura calcola e salva il CRC nella regione flash.
5. Il post-hook `BOOT_TO_INIT` effettua `HAL_NVIC_SystemReset()`. Al successivo
   avvio il bootloader ricontrolla il CRC prima di saltare.

Limiti del riferimento: il CRC viene generato dal destinatario dopo la
ricezione, senza CRC atteso dal mittente o lunghezza dichiarata nell'immagine.
Una chiusura anticipata può quindi produrre un CRC valido di dati incompleti.
Anche il SOES fissato dal F407 ignora il risultato di `on_foe_close()`.
Il porting replica il controllo di avvio e l'attivazione dopo reset, aggiungendo
verifiche del formato; non copia le operazioni ARM su C28.

## Percorso F28P65x prima della modifica

`main.c` configura la Flash API e inizializza `ecat_slv_init(&config)`.
SOES chiama il `bootstrap_foe_init()` dell'applicazione; i worker di mailbox
richiamano `ESC_foeprocess()` quando `USE_FOE=1`.
`ecat_options.h` usa mailbox da 128 ottetti: il payload FoE è di 116 ottetti.

`uc-test/common_src/soes_test/soes_hook.c` fornisce `config`. Il post-hook
INIT→BOOT chiama `jump_to_bootloader()`, ma il simbolo risolve alla funzione
weak che scrive soltanto un messaggio. Il linker originale colloca codestart
a `0x080000` e testo/costanti in bank 0; il codice FoE scrive bank 1.
Il normale codestart disabilita il watchdog e avvia `_c_int00`.
Nei sorgenti e nelle dipendenze fissate non si è trovato un validatore
F28P65x o un percorso di attivazione di bank 1. Questo non esclude un componente
esterno, un'immagine distinta o configurazioni hardware non accessibili.

| Proprietà | Verifica esistente |
| --- | --- |
| Nome/password | `f28p65x.bin`, `0xF280`; il confronto SOES accettava prefissi |
| Stato | File scrivibile soltanto in BOOT; FoE generale consente BOOT/PREOP |
| Lunghezza | Limite massimo e contatore `total_size`, nessuna lunghezza attesa |
| Buffer | 1024 contenitori di ottetti; ultimo buffer riempito con `0xFF` |
| Indirizzi | Conversione da ottetti a parole C28, limiti nel callback e nell'helper |
| Erase | Attesa FSM, controllo stato e blank check nell'helper uc-test |
| Programmazione | Attesa FSM, controllo stato e `Fapi_doVerify()` per blocco |
| Integrità immagine | Nessun confronto con CRC atteso nel percorso fissato |
| Chiusura/attivazione | Callback sempre positivo; SOES ne ignorava comunque il risultato |

`Calc_CRC()` nel flash helper F28P65x è uno stub che restituisce zero; la
dichiarazione usa inoltre `calc_CRC`. Il nuovo validatore non usa questo stub
né le firme dei parametri, che riguardano dati persistenti e non il firmware.
Le attese FSM non hanno timeout e il watchdog è disabilitato: un FSM bloccato
può bloccare il dispositivo. I valori di stato FSM, compreso il confronto con
`3U`, restano da verificare contro la versione FAPI e sul dispositivo.

## Implementazione del porting

Nuove configurazioni CCS: `BOOTLOADER`, `APPLICATION`,
`LAUNCHXL_BOOTLOADER`, `LAUNCHXL_APPLICATION`. Le configurazioni FLASH
preesistenti restano standalone e rifiutano FoE prima dell'erase.
Le nuove configurazioni derivano dalle configurazioni della rispettiva scheda;
nessuna è stata compilata con CCS in questo ambiente.

### Mappa flash CPU1

Tutti gli indirizzi della tabella sono **parole C28 da 16 bit**, non byte ARM.

| Regione | Inizio | Dimensione |
| --- | --- | --- |
| Bootloader, bank 0 | `0x080000` | `0x20000` parole / 256 KiB |
| Applicazione, bank 1 | `0x0A0000` | `0x1F800` parole / 252 KiB |
| Manifest, penultimo settore bank 1 | `0x0BF800` | `0x400` parole / 2 KiB |
| Commit, ultimo settore bank 1 | `0x0BFC00` | `0x400` parole / 2 KiB |
| Parametri, bank 2 | `0x0C0000` | partizione originale |
| Richiesta boot in RAM NOINIT | `0x0003FC` | 4 parole |

Il linker dell'applicazione pone codestart a `0x0A0000`, quindi testo e dati
di caricamento nella stessa partizione, senza accedere al manifest o al commit.
Il linker del bootloader mantiene codice e costanti in bank 0. Entrambi
riservano lo stesso mailbox RAM NOINIT, sottraendolo a RAMM0.

### File trasferito

`tools/pack_foe_image.py` accetta un binario piatto dell'applicazione collegata
a bank 1, in ordine **ottetto basso, ottetto alto per parola C28**. Lo completa
con `0xFF` fino a 252 KiB e aggiunge 32 ottetti di manifest. Il file FoE ha
esattamente **258080 ottetti**. Il padding dell'ultimo buffer FoE è esterno
alla lunghezza logica. Il settore commit non è accessibile ai dati FoE.

Manifest: otto interi little-endian da 32 bit:

| Offset in ottetti | Campo |
| --- | --- |
| 0 | Magic `0x46323850` |
| 4 | Versione formato `1` |
| 8 | Target CPU1: controlCARD `0xF28A6501`, LaunchPad `0xF28A6502` |
| 12 | Lunghezza effettiva payload, pari, 4..258048 ottetti |
| 16 | Entry C28 fissa `0x0A0000` |
| 20 | CRC-32/ISO-HDLC del payload effettivo |
| 24 | Flag riservati, zero |
| 28 | CRC-32/ISO-HDLC dei primi 28 ottetti del manifest |

CRC riflesso `0xEDB88320`, init/final XOR `0xFFFFFFFF`: coincide con
`zlib.crc32()`. È una nuova convenzione dichiarata, **non** il CRC hardware
STM32 alimentato a parole. Il CRC rileva corruzione accidentale, non autentica
il produttore e non impedisce downgrade o firmware ostile con CRC ricalcolato.
La password FoE non fornisce autenticità crittografica.

### Aggiornamento e reset

1. Il callback accetta WRQ solo in BOOT e con integrazione del bootloader.
   Controlla anche dimensione/allineamento del buffer prima di operare.
2. Cancella e verifica il settore commit **prima** di cambiare il payload.
   Poi cancella la regione applicativa. Bank 0 rimane il percorso di recupero.
3. Scrive sequenzialmente, controllando offset, partizione, unità di
   programmazione e risultato dell'helper sincrono con verifica.
4. Alla chiusura richiede la lunghezza logica esatta, verifica manifest,
   versione, target, entry, lunghezza payload, CRC e padding flash `0xFFFF`.
5. Scrive per ultimo un marcatore di 128 bit in un'unità ECC separata:
   magic, complemento, CRC manifest, complemento. Lo rilegge e ricontrolla
   l'immagine completa prima di restituire successo.
6. SOES manda l'ACK finale soltanto se flush e validazione sono riusciti.
   Il master esce da BOOT dopo quell'ACK; BOOT→INIT provoca reset.
7. All'avvio, prima di avviare EtherCAT e gli interrupt, il bootloader ripete
   i controlli. Con immagine valida salta a codestart C28; altrimenti avvia
   il servizio EtherCAT per recuperare l'immagine.

L'applicazione usa l'hook forte `jump_to_bootloader()`: scrive magic e
complemento in RAM NOINIT e richiede un watchdog reset con `SysCtl_resetDevice()`.
Il bootloader consuma la richiesta solo per reset watchdog senza POR e la
cancella. La richiesta serve solo a rimanere nel servizio FoE, non autorizza
mai l'esecuzione. La retention di questa RAM e la causa reset vanno provate
sulla scheda, anche dopo brownout e reset esterno.

Questo è un aggiornamento a singola applicazione con bootloader di recupero,
non A/B: dopo l'invalidazione non viene conservata l'app precedente. Non sono
implementati rollback, conferma del primo avvio o watchdog dell'applicazione.

## Correzioni SOES e dipendenze

Il branch SOES separato modifica `soes/esc_foe.c` e `soes/esc.c`:

- confronto esatto di nome e lunghezza, senza troncamenti né prefissi;
- propagazione del risultato del callback close;
- errore finale senza ACK sia per DATA corto sia per DATA terminale vuoto;
- uscita immediata dopo flush fallito, evitando l'ACK successivo all'abort;
- controllo lunghezza mailbox e gestione dell'errore di invio ACK finale;
- azzeramento della sessione quando vengono fermate le mailbox.

Il branch F28P65x fissa il nuovo commit SOES. I nuovi pre-build CCS e
`make foe-check` controllano gli hash dei due sorgenti SOES verificati.
Non modificare il manifest degli hash senza rivedere le nuove dipendenze e
rieseguire i test. `uc-test` rimane al commit originale: usa già blank check,
attesa FSM, controllo dello stato e readback della programmazione.

Non è implementato un timeout FoE inattivo: il master può mandare ERR o
riportare le mailbox a INIT e riaprire il file. Nessuna di queste interruzioni
pubblica un'immagine incompleta. Un pacchetto duplicato con numero inatteso
interrompe l'upload; non è implementata una nuova strategia di ritrasmissione.
Se si perde l'ACK finale dopo il commit, il master può vedere un errore mentre
l'immagine è già valida: al reset il bootloader la verifica comunque.

## Verifiche host e limiti

Eseguire con Python 3 e compilatore C host:

```sh
python3 tools/check_foe_stack.py
python3 tests/test_foe_update.py
```

I 17 test compilano ed eseguono i sorgenti di produzione `foe_utils.c`,
`esc_foe.c`, `boot_image.c`, con mailbox e flash simulate e contenitori
ottetto a 16 bit. Coprono successo e ricontrollo dopo reset, rifiuto prima
dell'erase, nome/password/stato, file vuoto/troncato, flush falliti, errore
erase, corruzione payload/manifest/padding, target/entry/lunghezze errate,
reset durante ricezione, invalidazione della vecchia immagine, commit
interrotto a ciascuna delle otto parole e abort/reset delle mailbox.

Non costituiscono un test del compilatore TI, della CPU C28, della Flash API,
dei bit FSM, delle condizioni elettriche né dell'esecuzione delle istruzioni
ricevute. Un CRC valido e un codestart non vuoto non dimostrano che l'app
sia eseguibile: occorre verificare link map e binario esportato.

## Build e prova sul Mac / scheda

I percorsi CCS obbligatori indicati da AGENTS.md non erano disponibili
nell'ambiente remoto usato per questo lavoro; nessuno strumento CCS o debug
è stato eseguito e nessuna scheda è stata programmata.

1. Importare le modifiche sui branch dedicati e inizializzare i submodule.
2. Compilare `BOOTLOADER` e `APPLICATION` (oppure entrambe le varianti
   `LAUNCHXL_...`) con CCS/C2000Ware/FAPI del progetto.
3. Controllare nelle mappe che codestart, tutte le sezioni di caricamento e
   le librerie rispettino le rispettive partizioni; che `.boot_request` sia
   NOINIT a `0x3FC`; che `.params` sia NOLOAD in bank 2.
4. Esportare soltanto l'applicazione in un binario piatto, con indirizzo base
   a parola `0xA0000` e ordine low/high corretto. Il precedente post-build
   citava `make_bin.cmd`, assente nel tree analizzato: non è stato riutilizzato
   come se fosse un export già verificato. Il packaging automatico da `.out`
   non è implementato; le nuove configurazioni non lanciano quel post-build.
5. Preparare il file: `python3 tools/pack_foe_image.py --board cc app-bank1.bin f28p65x.bin`
   (oppure `--board lp`). `make foe-package APP_BIN=... FOE_BOARD=cc` è equivalente.
6. Installare inizialmente il bootloader in bank 0 tramite debugger, dopo
   aver verificato che la politica di erase UniFlash preservi le altre bank.
   Con slot vuoto deve restare in EtherCAT e non saltare.
7. Portare in BOOT, caricare il file con password `0xF280`, attendere ACK
   finale, uscire da BOOT in INIT; verificare reset e ingresso nell'app.
8. Ripetere con CRC errato, firmware di altra scheda, file troncato e reset
   durante erase, ogni flush e commit. Verificare il recupero FoE da bank 0.
9. Provare il rientro dall'app, il reset watchdog, POR/brownout e il reset
   ESC; verificare RAM retention, flag reset e ripristino delle periferiche.
10. Verificare readback/ECC delle regioni cancellate e dei commit interrotti,
    gestione NMI per ECC non correggibile, stati FSM e tempi massimi. Prima
    dell'uso operativo servono timeout flash calibrati e percorso di recupero
    per un'app che supera CRC ma non si avvia correttamente.

## Sorgenti primarie

- [Introduzione FoE F28P65x](https://github.com/alessiomargan/f28p65x-soes/commit/e82658c5353ee280bdb121b281f04ff4b5b17a8b)
- [Callback originali F28P65x](https://github.com/alessiomargan/f28p65x-soes/blob/b69e2318108cd072f6b4ddc24c6a2053f10ab2b0/foe_utils.c)
- [SOES originale F28P65x](https://github.com/alessiomargan/SOES/blob/1fafe513eba8e07c5c5f1ea2aa4d430518ecf9cf/soes/esc_foe.c)
- [Flash helper F28P65x](https://github.com/alessiomargan/uc-test/blob/4605a23a62634d0afec380572a690ce4a1d9d03e/f28p65x/soes_test/flash_utils.c)
- [Hook e handoff originali](https://github.com/alessiomargan/uc-test/blob/4605a23a62634d0afec380572a690ce4a1d9d03e/common_src/soes_test/soes_hook.c)
- [Integrazione F407](https://github.com/alessiomargan/bl-stm32f407/blob/9a1a4a8dd2527a7bc0328a5a21fea924e8521456/cmake/soes.cmake)
- [Avvio e salto STM32](https://github.com/alessiomargan/bl-stm32f4xx-ecat/blob/72b497933ed5bf552dcd8e53800ebccb9d6bc295/user_code.c)
- [Chiusura upload STM32](https://github.com/alessiomargan/bl-stm32f4xx-ecat/blob/72b497933ed5bf552dcd8e53800ebccb9d6bc295/foe_utils.c)
- [Reset BOOT→INIT STM32](https://github.com/alessiomargan/bl-stm32f4xx-ecat/blob/72b497933ed5bf552dcd8e53800ebccb9d6bc295/soes_hook.c)
