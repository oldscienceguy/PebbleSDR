How to use EiBi frequency lists.
================================

OVERVIEW
A) Conditions of use.
B) How to use.
C) Available formats.
D) Codes used.

================================


A) Conditions of use.

All my frequency lists are free of cost, and any person
is absolutely free to download, use, copy, or distribute
these files or to use them within third-party software.
Should any mistake, omission or misinformation be found
in my lists, please send your helpful comment to
Eike.Bierwirth (at) gmx.de
[replace the "(at)" by the @ sign]


B) How to use.
   I) Try to identify a signal on the radio:
      You will need to know the frequency you tuned to in kHz, and the
      time of the reception in UTC.
      Search for the frequency in the list. Check all entries for this frequency:
       - The listening time should fall inbetween the start and end times given for the broadcast.
       - The day of the week should match. (If none is given in the list, the broadcast is daily.)
       - Try to guess the language you hear, if you don't know it.
       - Try to estimate the probability of propagation. The last column gives you information about
         the transmitter site (if not, the transmitter is in the home country of the station.)
         The target-area code should give you a clue if the signal is beamed in your direction or not.
         If you are right in the target area, great. If you are "behind" it (from the transmitter's
         point of view) or inbetween, your chances are also good. If the line connecting transmitter
         and target area is perpendicular to the line connecting the transmitter to your location,
         chances are lower. They are also lower if the target area is very close to the transmitter,
         while you are far away.
      Still, all rules can be turned upside-down by propagation conditions. You might sit in a good
      direction but the waves may skip 80 kilometers above you in the ionosphere. Listed information
      may be wrong, misleading, or outdated. Transmitters may fail, be upgraded, or drift in frequency.
      Transmitter sites may change (for example, fire in the control room, hurricane tore the antenna
      down, and transmissions have all been redirected to the Bonaire transmitter). Broadcasters and
      transmitter operators may mix up programmes or tune to the wrong frequency.
      My lists will be useful for fast identification of many stations on the band, but in a number of
      cases it will be of no use to you or it may bring you to a wrong conclusion. NO WARRANTY in case
      of a wrong ID, an embarrasingly wrong log or reception report sent out etc.
      
      The ONLY 100% ID is that which you hear on the radio yourself. Even QSLs may be wrong.
      
   II) Find a broadcast you would like to hear:
      Make extensive use of the SEARCH and SORT functions of the computer programme which you use.

      For text-format files I use TextPad, a freely downloadable text editor.
      Software has been written by very nice colleages of us to display the files in a more convenient manner,
      namely:
      
      ShortwaveLog, by Robert Sillett, N3OEA, which you can find on http://www.shortwavelog.com/
      
      RadioExplorer, by Dmitry Nefedov, Russia, which you can find on http://www.radioexplorer.com.ru/
      
      EiBiView, by Tobias Taufer, hosted on my website, real-time browsing!

      GuindaSoft Lister, freely downloadable from 
      http://guindasoft.impreseweb.com/utile/utile.html
      Click on the column headers to sort first by time, then by language or station or however you
      wish. Try it out.

      Not to forget the software of the PERSEUS receiver.

C) Available formats.

Since B06, the fundamental database is a CSV semicolon-separated list that contains
all the broadcasts relevant during the season, even if only for part of it.

A program script is used to transfer these data into the frequency-sorted and the
time-sorted text file. These two are the ones that exclusively contain the currently
valid data (current at the time of creation which is given at the top of the file).

Naming of files is
sked-Xzz.csv for the CSV database, load this into EiBiView;
freq-Xzz.txt for the frequency-sorted text file;
bc-Xzz.txt for the time-sorted text file;
where Xzz stands for the current broadcasting season:
 X=A: Summer season
 X=B: Winter season
 zz:  Two-character number for the year. E.g.,
 A99: Summer season 1999
 B06: Winter season 2006/2007.
 
The change from A to B or from B to A usually coincides with the
change of clock to or from Daylight Saving Time / Summer Time in many parts of
the world. The U.S.A., however, decided to use Daylight Saving Time
for an even longer period, beginning in 2007.

The download of freq and bc files shall furthermore be possible
in PDF, DOC, and zipped format.

A file called eibi.txt is the same as the freq file, copy this into
your Perseus directory to view the EiBi entries in that receiver's software.

Format of the CSV database (sked-):
Entry #1 (float between 150 and 30000): Frequency in kHz
Entry #2 (9-character string): Start and end time, UTC, as two '(I04)' numbers separated by a '-'
Entry #3 (string of up to 5 characters): Days of operation, or some special comments:
         Mo,Tu,We,Th,Fr,Sa,Su - Days of the week
         1245 - Days of the week, 1=Monday etc. (this example is "Monday, Tuesday, Thursday, Friday")
         irr - Irregular operation
         alt - Alternative frequency, not usually in use
         Ram - Ramadan special schedule
         Haj - Special broadcast for the Haj
         15Sep - Broadcast only on 15 September
         RSHD - Special broadcast on Radio St.Helena Day, once a year, usu. Oct/Nov
         I,II,etc. - Month in Roman numerals (IX=September etc.)
Entry #4 (string up to 3 characters): ITU code (see below) of the station's home country
Entry #5 (string up to 23 characters): Station name
Entry #6 (string up to 3 characters): Language code
Entry #7 (string up to 3 characters): Target-area code
Entry #8 (string): Transmitter-site code
Entry #9 (integer): Persistence code, where:
         1 = This broadcast is everlasting (i.e., it will be copied into next season's file automatically)
         2 = As 1, but with a DST shift (northern hemisphere)
         3 = As 1, but with a DST shift (southern hemisphere)
         4 = As 1, but active only in the winter season
         5 = As 1, but active only in the summer season
         6 = Valid only for part of this season; dates given in entries #10 and #11. Useful to re-check old logs.
         8 = Inactive entry, not copied into the bc and freq files.
Entry #10: Start date if entry #9=6. 0401 = 4th January. Sometimes used with #9=1 if a new permanent service is started, for information purposes only.
Entry #11: As 10, but end date.


D) Codes used.
   I)   Language codes.
   II)  Country codes.
   III) Target-area codes.
   IV)  Transmitter-site codes.
   
   I) Lanuage codes.
   
   Numbers are number of speakers. 4m = 4 millions.
   On the right in [brackets] the German translation, if significantly different.
   
   
   -TS   Time Signal Station                                   [Zeitzeichensender]
   A     Arabic (300m)                                                  [Arabisch]
   AB    Abkhaz: Georgia-Abkhazia (0.1m)                              [Abchasisch]
   AC    Aceh: Indonesia-Sumatera (3m)
   ACH   Achang / Ngac'ang: Myanmar, South China (30,000)
   AD    Adygea / Adyghe / Circassic: Russia-Caucasus (0.5m)
   ADI   Adi: India-Assam (0.4m)
   AF    Afrikaans: South Africa, Namibia (6.4m)
   AFA   Afar: Djibouti (0.3m), Ethiopia (0.45m), Eritrea (0.3m)
   AFG   Pashto and Dari (main Afghan languages)
   AH    Amharic: Ethiopia (20m)                                       [Amharisch]
   AJ    Adja / Aja-Gbe: Benin, Togo (0.5m)
   AK    Akha: Burma (0.2m), China-Yunnan (0.13m)
   AL    Albanian: Albania (Tosk)(3m), Macedonia/Yugoslavia (Gheg)(2m) [Albanisch]
   ALG   Algerian (Arabic): Algeria (35m)
   AM    Amoy: S China (25m), Taiwan (15m), SoEaAsia (5m); very close to TW-Taiwanese
   AMD   Tibetan Amdo (Tibet: 0.8m)
   Ang   Angelus programme of Vaticane Radio
   AR    Armenian: Armenia (3m), USA (1m), RUS,GEO,SYR,LBN,IRN,EGY     [Armenisch]
         AR-W: Western, AR-E: Eastern
   ARO   Aromanian: Greece, Albania, Macedonia (0.1m)
   ARU   Languages of Arunachal (India)
   ASS   Assamese: Assam/India (14m)
   ASY   Assyrian: Middle East (0.1m)                                  [Assyrisch]
   ATS   Atsi / Zaiwa: Myanmar (13,000), China (70,000)
   Aud   Papal Audience (Vaticane Radio)                     [Audienz des Papstes]
   AV    Avar: Dagestan, S Russia (0.6m)                                [Awarisch]
   AW    Awadhi: N&Ce India (20m)
   AY    Aymara: Bolivia (2m)
   AZ    Azeri: Azerbaijan (7m)                                [Aserbaidschanisch]
   B     Brazilian (Portuguese dialect)                     [Brasil.Portugiesisch]
   BAD   Badaga: India-Tamil Nadu (0.17m)
   BAG   Bagri: India (1.7m), Pakistan (0.2m)
   BAJ   Bajau: Malaysia-Sabah (50,000)
   BAL   Balinese: Bali / Indonesia (4m)                             [Balinesisch]
   BAN   Banjar/Banjarese: Indonesia/Borneo (3m)
   BAO   Baoule: Cote d'Ivoire (2m)
   BAR   Bari: S Sudan (0.3m)
   BAS   Bashkir:  Ce Russia (1m)                                   [Baschkirisch]
   BAY   Bayash/Boyash Romani: Serbia, Croatia (10,000)
   BB    Braj Bhasa/Braj Bhasha/Brij: India (40,000)
   BC    Baluchi: Pakistan (1m), Iran (0.5m), Afghanistan (0.2m)
   BE    Bengali/Bangla: Bangladesh (100m), India (68m)
   BED   Bedawi / Beja: Sudan (1m)
   BEM   Bemba: Zambia (4m)
   BGL   Bagheli: N India (400,000)
   BH    Bhili: Ce India (1.6m)
   BHN   Bahnar: Vietnam (170,000)
   BHT   Bhatri: India-Chhattisgarh,Maharashtra (0.6m)
   BI    Bile: Eritrea-Keren (70,000)
   BID   Bidayuh languages: Malaysia-Sarawak (150,000)
   BIS   Bisaya: Malaysia-Sarawak,Sabah (20,000), Brunei (40,000)
   BJ    Bhojpuri/Bihari: India (23m), Nepal (1.4m)
   BK    Balkarian: Russia-Caucasus (0.4m)
   BLK   Balkan Romani: Bulgaria (0.4m), Serbia (0.1m), Macedonia (0.1m)
   BLT   Balti: Pakistan (0.3m)
   BM    Bambara: Mali (2.7m)
   BNA   Borana Oromo: Ethiopia (4m)
   BNG   Bangala / Mbangala: Central Angola (22,000)
   BNJ   Banjari / Banjara / Gormati / Lambadi: India-Maharashtra,Karnataka,Andhra Pr.(2m)
   BNT   Bantawa: Nepal (400,000)
   BON   Bondo: India-Orissa (9000)
   BOR   Boro / Bodo: India-Assam,W Bengal (0.5m)
   BOS   Bosnian (derived from Serbocroat): Bosnia-Hercegovina (4m)     [Bosnisch]
   BR    Burmese / Barma / Myanmar: Myanmar (40m)                     [Birmanisch]
   BRA   Brahui: Pakistan (1.5m), Afghanistan (0.2m)
   BRB   Bariba / Baatonum: Benin, Nigeria (560,000)
   BRU   Bru: Laos (70,000), Vietnam (55,000)
   BSL   Bislama: Vanuatu (0.2m)
   BT    Black Tai / Tai Dam: Vietnam (0.5m)
   BTK   Batak-Toba: Indonesia-Sumatra (2m)
   BU    Bulgarian: Bulgaria (8m), Moldova (0.36m), Ukraine (0.2m)    [Bulgarisch]
   BUG   Bugis / Buginese: Indonesia-Celebes (4m)
   BUK   Bukharian/Bukhori (Jewish dialect of Farsi): Israel (50,000), Uzbekistan (10,000)
   BUN   Bundeli / Bundelkhandi: India-Uttar,Madhya Pr. (1m)
   BUR   Buryat: South Siberia, similar to Mongolian (0.4m)           [Burjatisch]
   BY    Byelorussian / Belarusian: Belarus, Poland, Ukraine (9m)  [Weissrussisch]
   C     Chinese (not further specified)                              [Chinesisch]
   CA    Cantonese / Yue  (South China 50m, others 15m)             [Kantonesisch]
   CC    Chaochow (South China, close to TW-Taiwanese, AM-Amoy, SWT-Swatow)
   CD    Chowdary/Chaudhry/Chodri: India-Gujarat (0.3m)
   CEB   Cebuano: Philippines (20m)
   CH    Chin (not further specified): Myanmar
   C-A   Chin-Asho: Myanmar-Arakan Hills (10,000)
   C-D   Chin-Daai: Myanmar (30,000)
         Chinyanja: See NY-Nyanja
   C-F   Chin-Falam / Halam: Myanmar, Bangladesh, India (125,000)
   CHA   Cha'palaa (Ecuador)
   CHE   Chechen: Russia-Chechnya (1.5m)                      [Tschetschenisch]
   CHG   Chattisgarhi: India-Mad.Pr.,Orissa,Bihar (11m)
   CHI   Chitrali / Khowar: NW Pakistan (0.4m)
   C-K   Chin-Khumi: Myanmar (77,000)
   C-M   Chin-Mro: Myanmar (130,000)
   C-O   Chin-Thado / Thadou-Kuki: India-Assam, Manipur (0.15m)
   CHR   Chrau: Vietnam (20,000)
   CHU   Chuwabu: Mozambique (1m)
   C-T   Chin-Tidim: Myanmar (0.2m), India (0.15m)
   C-Z   Chin-Zomin / Zomi-Chin: Myanmar (30,000), India-Manipur (20,000)
   CKM   Chakma (India)
   CKW   Chokwe: Angola (0.5m), DR Congo (0.5m)
   COF   Cofan (Ecuador)
   COK   Cook Islands Maori
   CR    Creole  (Caribbean)                                           [Kreolisch]
   CRU   Chru: Vietnam (15,000)
   CT    Catalan: Spain, Andorra (31000)                             [Katalanisch]
   CV    Chuvash (Chuvashia/Russia)                               [Tschuwaschisch]
   CW    Chewa /Chichewa (Malawi)
   CZ    Czech                                                       [Tschechisch]
   D     German                                                          [Deutsch]
   D-P   Lower German (Northern Germany, USA)                       [Plattdeutsch]
   DA    Danish                                                         [Daenisch]
   DAO   Dao: Vietnam (0.5m)
   DD    Dhodiya / Dhodia: India-Gujarat (150,000)
   DEG   Degar / Montagnard (Vietnam)
   DEN   Dendi (Benin, Nigeria: 32,000)
   DEO   Deori (India - Assam: 27,000)
   DES   Desiya / Deshiya / Adivasi Oriya (India - Andhra Pr.: 150,000)
   DH    Dhivehi (Maldives)
   DI    Dinka (Sudan)
   DIA   Dial Arabic (North Africa)
   DIM   Dimasa/Dhimasa (India-Assam: 0.1m)
   DIT   Ditamari (Benin, Togo: 50,000) 
   DN    Damara-Nama service in Namibia (Languages: Damara, Nama)
   DO    Dogri-Kangri: N India (2m)
   DR    Dari / Eastern Farsi: Afghanistan (6m), Pakistan (1m)
   DU    Dusun (Malaysia-Sabah)
   DUN   Dungan (Central Asia)
   DY    Dyula (Burkina, Ivory Coast, Mali)
   DZ    Dzonkha: Bhutan (0.4m)
   E     English
   EC    Eastern Cham: Vietnam (70,000)
   EDI   English, German, Italian
   E-C   Col.English (S Sudan)
   EGY   Egyptian Arabic
   E-L   English Learning Programme (BBC)                    [Englisch-Sprachkurs]
   EO    Esperanto
   ES    Estonian                                                       [Estnisch]
   Ewe   Ewe (Ghana)
   F     French                                                     [Franzoesisch]
   FA    Faroese                                                     [Faeringisch]
   FI    Finnish                                                        [Finnisch]
   FJ    Fijian
   FON   Fon / Fongbe (Benin, Togo: 1.7m)
   FP    Filipino
   FS    Farsi / Persian                                                [Persisch]
   FT    Fiote (Angola-Cabinda)
   FU    Fulani/Fulfulde (0.25m BFA, 0.3m BEN, 5m CME)
   FUJ   FutaJalon (West Africa)
         Fujian: see TW-Taiwanese
   FUR   Fur (0.5m Sudan, Darfur)
   GA    Garhwali: India-Kashmir,Uttar Pr. (2m)
   GAR   Garo (India, Bangladesh: 700,000)
   GD    Greenlandic                                              [Groenlaendisch]
   GE    Georgian                                                      [Georgisch]
   GJ    Gujari/Gojri (India/Pakistan)
   GL    Galicic/Gallego (Spain)                                      [Gallizisch]
   GM    Gamit: India-Gujarat (0.2m)
   GNG   Gurung: Nepal (0.2m)
   GO    Gorontalo (Indonesia)
   GON   Gondi (India-Madhya Pr.,Maharashtra: 3m)
   GR    Greek                                                        [Griechisch]
   GU    Gujarati: India (43m)
   GUA   Guaraní (Paraguay)
   GUR   Gurage/Guragena: Ethiopia (1m)
   GZ    Ge'ez / Geez (liturgic language of Ethiopia)
   HA    Haussa  (Nigeria)
   HAD   Hadiya (Ethiopia)
   HAR   Haryanvi /  Bangri / Harayanvi / Hariyanvi (India: 13m)
   HB    Hebrew  (Israel)                                             [Hebraeisch]
   HD    Hindko (Pakistan)
   HI    Hindi: India (180m)
   HK    Hakka (South China 26m, TWN 2m, others 6m)
         Hokkien: see TW-Taiwanese
   HM    Hmong (Vietnam)
   HMA   Hmar (India - Assam,Manipur,Mizoram 50,000)
   HMB   Hmong-Blue/Njua (Laos/Thailand)
   HMQ   Hmong, Northern Qiandong / Black Hmong (China: 900,000)
   HMW   Hmong-White/Daw (Laos/Thailand)
   HN    Hani (S China)
   HO    Ho (India)
   HR    Croatian (Hrvatski)                                           [Kroatisch]
   HRE   Hre: Vietnam (0.1m)
   HU    Hungarian                                                     [Ungarisch]
   HUA   Huarani (Ecuador)
   HUI   Hui / Huizhou: China - Anhui (3m)
   HZ    Hazaragi: Afghanistan (1.4m), Iran (0.3m)
   I     Italian                                                     [Italienisch]
   IB    Iban (Malaysia - Sarawak)
   Ibo   Ibo (Africa)
   IF    Ifè / Ife (Benin, Togo: 155,000)
   IG    Igbo (Nigeria)
   ILC   Ilocano (Philippines)
   ILG   Ilonggo (Philippines)
   IN    Indonesian / Bahasa Indonesia: Indonesia (140m)             [Indonesisch]
   INU   Inuktikut (Inuit language, Canada/Greenland)
   IRQ   Iraqi
   IS    Icelandic                                                   [Islaendisch]
   J     Japanese                                                      [Japanisch]
   JEH   Jeh: Vietnam (15,000), Laos (8,000)
   JG    Jingpho: see KC-Kachin
   JL    Joula/Jula (West Africa)
   JOR   Jordanian (Arabic)
   JR    Jarai / Giarai / Jra: Vietnam (0.3m)
   JU    Juba (Arabic; Chad, Sudan)
   JV    Javanese: Indonesia-Java,Bali (84m)
   K     Korean                                                       [Koreanisch]
   KA    Karen (unspecified): Myanmar (3m)
   K-G   Karen-Geba: Myanmar (10,000)
   K-K   Karen-Geko / Gekho: Myanmar (10,000)
   K-P   Karen-Pao: Myanmar (0.5m)
   K-S   Karen-Sgaw: Myanmar (1.5m)
   K-W   Karen-Pwo: Myanmar (1m)
   KAD   Kadazan (Malaysia - Sabah)
   KAL   Kalderash Romani (Gypsy) (Romania)                         [Roma-Sprache]
   KAB   Kabardino (Caucasus)
   KAM   Kambaata (Ethiopia)
   KAN   Kannada (South India)
   KAO   Kaonde (Zambia)
   KAR   Karelian (Russia, close to Finnish border).                   [Karelisch]
         Programme includes Finnish and Vespian parts.
   KAT   Katu: Vietnam (50,000)
   KAU   Kaubru (India)
   KAY   Kayan: Myanmar (40,000)
   KB    Kabyle: Algeria (2.5m)                                 [Kabylisch/Berber]
   KBO   Kok Borok/Tripuri: India (0.7m)
   KC    Kachin / Jingpho: Myanmar (1m)
   KG    Kyrgyz /Kirghiz: Kyrgystan (2.5m), China (0.1m)              [Kirgisisch]
   KH    Khmer: Cambodia (6m), Vietnam (0.7m)                    [Kambodschanisch]
   KHA   Kham / Khams, Eastern (NE Tibet: 1.5m)
   KHM   Khmu (Laos 400,000, Vietnam 40,000)
   KHR   Kharia / Khariya (India, Nepal: 300,000)
   KHS   Khasi / Kahasi (India, Bangladesh: 0.9m)
   KHT   Khota (India)
   KIM   Kimwani (East Africa)
   KIN   Kinnauri / Kinori (India - Himachal Pr.: 50,000)
   KiR   KiRundi: Burundi (0.45m)
   KK    KiKongo/Kongo: DR Congo, Angola (3.5m)
   KKN   Kukna (India - Gujarat: 0.6m)
   KMB   Kimbundu/Mbundu/Luanda: Angola (3m)
   KMY   Kumyk (Dagestan / Russia)
   KND   Khandesi (India - Maharashtra: 1.5m)
   KNK   KinyaRwanda-KiRundi service of the Voice of America / BBC
   KNU   Kanuri: Nigeria (3m), Chad (0.1m), Niger,Cameroon (0.1m)
   KNY   Konyak Naga (India - Assam, Nagaland: 0.1m)
   KO    Kosovar Albanian (Kosovo/Serbia)
   KOH   Koho/Kohor: Vietnam (0.1m)
   KOK   Kokang Shan: Myanmar
   KOM   Komering (Indonesia)
   KON   Konkani (India)
   KOY   Koya (India-Andhra Pr.: 300,000)
   KPK   Karakalpak (W Uzbekistan)
   KRB   Karbi / Mikir / Manchati (NE India: 0.5m)
   KRI   Krio (Sierra Leone)
   KRW   KinyaRwanda (Rwanda)
   KS    Kashmiri: India (4m), Pakistan (0.1m)
   KT    Kituba (simplified Kikongo): Rep. Congo (1m), DR Congo (4m)
   KTW   Kotwali (India)
   KU    Kurdish
   KuA   Kurdish and Arabic
   KuF   Kurdish and Farsi
   KUI   Kui (India)
   KUK   Kukana (India)
   KUL   Kulina (Brazil)
   KUM   Kumaoni/Kumauni (India)
   KUN   Kunama: Eritrea (0.14m)
   KUP   Kupia / Kupiya (India - Andhra Pr.: 4,000)
   KUR   Kurukh: India (2m)
   KUs   Sorrani Kurdish
   KUT   Kutchi (India 800,000)
   KVI   Kulluvi (India)
   KWA   Kwanyama/Kuanyama (Ce Angola)
   KYH   Kayah: Myanmar (0.1m)
   KYK   Kayan/Kenyah (Malaysia-Sarawak)
   KZ    Kazakh: Kazakhstan (7m), China (1m), Mongolia (0.1m)         [Kasachisch]
   L     Latin                                                        [Lateinisch]
   LA    Ladino (Hispanic Jewish lang.)
   LAD   Ladakhi / Ladak (India - Jammu+Kashmir: 0.1m)
   LAH   Lahu: China (0.4m), Myanmar (150,000)
   LAK   Lak (Dagestan / Russia)
   LAM   Lampung (Indonesia)
   LAO   Lao                                                            [Laotisch]
   LaS   Ladino and Spanish
   LB    Lun Bawang / Murut (Malaysia-Sarawak)
   LBN   Lebanon Arabic
   LBO   Limboo /Limbu (NE India)
   LEP   Lepcha (NE India)
   LEZ   Lezgi (Dagestan / Russia: 0.4m)
   LHO   Lhotshampa (Bhutan)
   LIM   Limba (Sierra Leone)
   LIN   Lingala (Congo)
   LIS   Lisu: China-West Yunnan (0.6m), Burma (0.1m)
   LND   Lunda Ndembo (Angola)
   LNG   Lungeli-Magar (India)
   LO    Lomwe (Mocambique)
   LOK   Lokpa / Lukpa (Benin, Togo: 65,000)
   LOZ   Lozi / Silozi (Zambia 500,000, ZWE+BOT 100,000)
   LT    Lithuanian                                                    [Litauisch]
   LTO   Oriental Liturgy of Vaticane Radio                      [Oestl.Lithurgie]
   LU    Lunda (Angola)
   LUB   Luba: DR Congo (6m)
   LUC   Luchazi: Angola (0.24m), Zambia (0.05m)
   LUG   Luganda: Uganda (5m)
   LUN   Lunyaneka/Nyaneka: Angola (0.4m)
   LUR   Luri (Iran)
   LUV   Luvale (Angola)
   LV    Latvian                                                        [Lettisch]
   M     Mandarin (Standard Chinese / Beijing dialect)
   MA    Maltese                                                      [Maltesisch]
   MAD   Madurese/Madura: Indonesia-Java,Bali (14m)
   MAG   Maghi/Magahi/Maghai (India)
   MAI   Maithili / Maithali (India: 22m, Nepal: 2m)
   MAK   Makonde (Tanzania/Mocambique)
   MAL   Malayalam (South India)
   MAM   Mamay / Rahanwein (Somalia)
   MAN   Manchu  (Vietnam)
   MAO   Maori  (New Zealand)
   MAR   Marathi (India)
   MAS   Masaai/Massai/Masai (E Africa)
   MC    Macedonian (2m)                                             [Makedonisch]
   MCH   Mavchi/Mouchi/Mauchi/Mawchi (India-Gujarat,Maharashtra: 72,000)
   MEI   Meithei/Manipuri/Meitei: India (1.3m)
   MEN   Mende: Sierra Leone (1.5m)
   MEW   Mewari/Mewadi (Rajasthani): India-Rajasthan (5m)
   MGA   Magar: Nepal (0.8m)
   MIE   Mien / Iu Mien: S China (0.5m), Vietnam (0.3m)
   MIS   Mising (dialect of ADI): India-Assam (25,000)
   MKB   Minangkabau: Indonesia-West Sumatra (6.5m)
   MKS   Makassar/Makasar: Indonesia-South Sulawesi (2m)
   MKU   Makua / Makhuwa: N Mocambique (2.5m)
   ML    Malay / Baku: Malaysia, Singapore, Brunei (18m)               [Malaiisch]
   MLK   Malinke/Maninka: Guinea, Liberia, Sierra Leone (2m)
   MLT   Malto / Kumarbhag Paharia: India- Orissa, West Bengal (20,000)
   MNE   Montenegrin: Montenegro (mutually intelligible with SR,HR,BOS)
   MNO   Mnong: Vietnam (0.1m)
   MO    Mongolian / Halh: Mongolia (2m)                              [Mongolisch]
   MOc   Chinese / Peripheral Mongolian (N China: 3m)  
   MON   Mon: Myanmar (1m)
   MOO   Moore: Burkina Faso (4.5m)
   MOR   Moro/Moru/Muro: Southern Sudan (0.1m)
   MR    Maronite / Cypriot Arabic: Cyprus (1300)
   MRC   Moroccan/Mugrabian (Arabic): Morocco (20m)
   MRI   Mari: Russia-Mari (0.6m)
   MRU   Maru / Lhao Vo: Burma (0.1m)
   MSY   Malagasy: Madagaskar (20m)                                 [Madegassisch]
   MUN   Mundari: India (1.5m)
   MUO   Muong: Vietnam (1m)
   MUR   Murut: Malaysia-Borneo (11,000)
   MW    Marwari (Rajasthani): India (13m)
   MY    Maya (Yucatec): Mexico (0.8m)
   MZ    Mizo / Lushai: India-Mizoram (0.7m)
   NAG   Naga / Ao /Makware: India - Nagaland, Assam (140,000)
   NDA   Ndau: Mocambique (0.6m)
   NDE   Ndebele: Zimbabwe (1.5m), South Africa-Transvaal (0.6m)
   NE    Nepali: Nepal (10m), India (6m)                             [Nepalesisch]
   NET   Netekani (India)
   NG    Nagpuri / Sadani / Sadari / Sadri (NE India: 2m)
   NGA   Ngangela/Nyemba (Angola)
   NI    Ni (South Asia)
   NIC   Nicobari (Nicobar Islands, India)
   NIS   Nishi/Nyishi (India-Arunachal Pradesh: 0.3m)
   NIU   Niuean (Niue / S Pacific)
   NL    Dutch (Nederlands)                                      [Niederlaendisch]
         Flemish is included here ;-)
   NLA   Nga La: Myanmar (40,000)
   NO    Norwegian                                                    [Norwegisch]
   NOC   Nocte / Nockte (India - Assam, Arunachal Pr.: 35,000)
   NP    Nupe (Nigeria)
   NTK   Natakani / Netakani, dialect of Varhadi-Nagpuri (S India, 7m)
   NU    Nuer: Sudan (0.75m), Ethiopia (0.04m)
   NUN   Nung: Vietnam (0.8m)
   NW    Newari (India)
   NY    Nyanja / Chinyanja (Malawi 3m, Zambia 1m, ZWE+MOZ 1m)
   OG    Ogan (Indonesia)
   OH    Otjiherero service in Namibia (Languages: Herero, SeTswana)
   OO    Oromo: Ethiopia (8m)
   OR    Oriya / Orissa (India: 32m)
   OS    Ossetic (Caucasus)                                            [Ossetisch]
   OW    Oshiwambo service in Angola and Namibia (Languages: Ovambo, Kwanyama)
   P     Portuguese                                                [Portugiesisch]
   PAL   Palaung - Pale: Burma (0.3m)
   PAS   Pasemah (Indonesia)
   PED   Pedi (S Africa)
   PF    Pashto and Farsi
   PJ    Punjabi (India/Pakistan)
   PO    Polish (Poland)                                                [Polnisch]
   POR   Po: Myanmar-Rakhine (identical to K-W?)
   POT   Pothwari (Pakistan)
   PS    Pashto / Pushtu: Afghanistan (8m), Iran (0.1m)
   PU    Pulaar (West Africa)
   Q     Quechua: Bolivia (2.8m)
   QQ    Qashqai: SW Iran (1.5m)
   R     Russian                                                        [Russisch]
   RAD   Rade/Ede: Vietnam (0.3m)
   REN   Rengao (Vietnam)
   RGM   Rengma (India - Nagaland: 21,000)
   RH    Rahanwein (Somalia)
   RO    Romanian                                                     [Rumaenisch]
   ROG   Roglai: Vietnam (50,000)
   ROM   Romanes (Gypsy)                                            [Roma-Sprache]
   RON   Rongmei (India - Manipur, Nagaland, Assam: 60,000)
   Ros   Rosary session of Vaticane Radio                             [Rosenkranz]
   RWG   Rawang: Burma (60,000), India-Arunachal Pr.(60,000)
   S     Spanish                                                        [Spanisch]
   SAH   Saho: Eritrea, South (0.14m)
   SAM   Samayiki (India)
   SAN   Sango (Central African Rep.)
   SAS   Sasak: Indonesia-Lombok (2m)
   SC    Serbo-Croat (Yugoslav language before the                [Serbokroatisch]
                national and linguistic separation)
   SCA   Scandinavian languages (Norwegian, Swedish, Finnish)
   SD    Sindhi: Pakistan (17m), India (3m)
   SED   Sedang: Vietnam (0.1m)
   SEF   Sefardi (Jewish language in Spain)
   SEN   Sena (Mocambique)
   SGA   Shangaan (Mocambique)
   SGM   Sara Gambai / Sara Ngambai (Chad)
   SGO   Songo (Angola)
   SHA   Shan: Burma (3m)
   SHk   Shan-Khamti (Burma)
   SHC   Sharchogpa / Sarchopa (Bhutan)
   SHE   Sheena (Pakistan)
   SHK   Shiluk (S Sudan: 175,000)
   SHO   Shona (Zimbabwe)
   SHP   Sherpa (India,Nepal: 30,000)
   SHU   Shuwa (Arabic of Central Africa, Chad)
   SI    Sinhalese (Sri Lanka)
   SID   Sidamo/Sidama (Ethiopia, Eritrea)
   SIK   Sikkimese (NE India)
         Silozi: See LOZ-Lozi
   SIR   Siraiki (Pakistan)
   SK    Slovak                                                       [Slowakisch]
   SLM   Solomon Islands Pidgin (Solomon Isl., S.Pacific)
   SLT   Silte / East Gurage (Ethiopia: 1m)
   SM    Samoan: Samoa (0.2m), USA (0.1m)
   SMP   Sambalpuri / Sambealpuri (dialect of Oriya) (India - Orissa)
   SNK   Sanskrit (India)
   SNT   Santhali: India (5.5m), Bangladesh (0.15m)
   SO    Somali
   SON   Songhai: Mali (0.6m), Niger (0.4m), Burkina (0.1m)
   SOT   SeSotho (Lesotho)
   SOU   Sous (North Africa)
   SR    Serbian                                                        [Serbisch]
   SRA   Soura / Saurashtra (India-Tamil N.,Karnataka,Andhra Pr.: 300,000)
   STI   Stieng: Vietnam (50,000)
   SUA   Shuar (Ecuador)
   SUD   Sudanese (Arabic)
   SUM   Sumi (India - Nagaland: 0.1m)
   SUN   Sunda/Sundanese: Indonesia-West Java (34m)
   SUS   Sousou (West Africa)
   SV    Slovenian                                                    [Slowenisch]
   SWA   Swahili  (Kenya/Tanzania/Uganda)                              [Kisuaheli]
   SWE   Swedish                                                      [Schwedisch]
   SWT   Swatow (China)
   SWZ   SiSwati (Swaziland)
   SYL   Syrian-Lebanese Arabic
   T     Thai
   TAG   Tagalog (Philippines)
   TAH   Tachelhit (North Africa)
   TAM   Tamil (South India / Sri Lanka)                               [Tamilisch]
   TB    Tibetan: Tibet (1m), India (0.1m)                             [Tibetisch]
   TBS   Tabasaran: Dagestan / Russia (0.1m)
   TEL   Telugu (South India)
   TEM   Temme (Sierra Leone)
   TFT   Tarifit (North Africa)
   TGK   Tangkhul/Tangkul (India - Manipur, Nagaland: 0.1m)
   TGR   Tigre/Tigré (Eritrea: 800,000) (not = Tigrinya)
   TGS   Tangsa/Naga-Tase (Myanmar, India-Arunachal Pradesh: 0.1m)
   THA   Tharu Buksa (India-Nainital: 20,000)
   TIG   Tigrinya/Tigray (Ethiopia: 3m, Eritrea: 2m)
   TJ    Tajik                                                      [Tadschikisch]
   TK    Turkmen                                                     [Turkmenisch]
   TL    Tai-Lu/Lu/Xishuangbanna Dai: S China (0.3m), Burma (0.2m), Laos (0.1m)
   TM    Tamazight (North Africa)
   TMG   Tamang (Nepal: 0.3m)
   TMJ   Tamajeq (West Africa)
   TN    Tai-Nua/Chinese Shan/Dehong Dai: S China (250,000), Laos/Burma (0.2m)
   TNG   Tonga (Zambia)
   TO    Tongan (Tonga, So.Pacific)
   TOK   Tokelau (Tokelau, So.Pacific)
   TOR   Torajanese (Indonesia)
   TP    Tok Pisin (PNG pidgin)
   TS    Tswana / SeTswana: Botswana (1m), South Africa (3m)
   TSA   Tsangla: Bhutan (0.4m). Close to Sharchagpakha / Sarchopa
   TSH   Tshwa (Mocambique)
   TT    Tatar: Central Russia, Volga (7m)
   TTB   Tatar-Bashkir service of Radio Liberty
   TU    Turkish: Turkey (46m), Bulgaria (0.8m)                        [Tuerkisch]
   TUL   Tulu (South India)
   TUM   Tumbuka (Malawi)
   TUN   Tunisian (Arabic)
   TUR   Turki
   TV    Tuva / Tuvinic  (Tannu-Tuva, S Siberia) and Russian           [Tuwinisch]
   TW    Taiwanese / Fujian / Hokkien (CHN 25m, TWN 15m, others 9m)
   Twi   Twi/Akan: Ghana (7m - Ashanti language)
   TWT   Tachawit/Shawiya/Chaouia: Algerian (2m)
   TZ    Tamazight/Berber: Morocco (2m), Algeria (1m)
   UD    Udmurt (Ce Russia)
   UI    Uighur: China/Xinjiang (7m), Kazakhstan (0.2m)                [Uigurisch]
   UK    Ukrainian                                                    [Ukrainisch]
   UM    Umbundu: Angola (4m)
   UR    Urdu: Pakistan (54m)
   UZ    Uzbek (Southern Variant: 1m in Afghanistan)                   [Usbekisch]
   V     Vasco (Basque / Spain)                                         [Baskisch]
   VAD   Vadari / Waddar / Od (India - Andhra Pr.: 2m)
   VAR   Varli / Warli (India - Maharashtra: 0.6m)
   Ves   Vespers (Vaticane Radio)                                         [Vesper]
   Vn    Vernacular = local languages (maybe various)               [Lokalsprache]
   VN    Vietnamese                                                [Vietnamesisch]
   VV    Vasavi (India)
   VX    Vlax Romani (Balkans)
   W     Wolof (Senegal)
   WA    Wa: South China (Vo Wa, 40,000), Myanmar (Parauk Wa, 1m)
   WAO   Waodani/Waorani (South America)
   WE    Wenzhou (China)
   WT    White Tai / Tai Don (Vietnam,Laos: 400,000)
   WU    Wu (China - Jiangsu, Zhejiang: 80m)
   XH    Xhosa (South Africa)
   YAO   Yao (Mocambique)
   YER   Yerukula (India - Andhra Pr.: 0.3m)
   Yi    Yi / Nosu (China-Sichuan,Yunnan: 6 Yi dialects of each 300-800,000)
   YI    Yiddish (Jewish)                                               [Jiddisch]
   YK    Yakutian / Sakha (Rep.Sakha, Siberia/Russia)                  [Jakutisch]
   YO    Yoruba: Nigeria (20m)
   YOL   Yolngu (NE Arnhem Land, NT, Australia; 7000 speakers)
   YU    Yu (East Asia, ?)
   YZ    Yezidish (Armenia)
   Z     Zulu (South Africa)
   ZA    Zarma/Zama (West Africa)
   ZD    Zande (S Sudan)
   ZG    Zaghawa (Chad)
   ZH    Zhuang: S China (2 Zhuang dialects of 10m and 4m)
   ZWE   Languages of Zimbabwe
         Zomi-Chin: see Chin-Zomi (C-Z)

   II) Country codes.
   Countries are referred to by their ITU code. Here is a list:
   Asterisks (*) denote non-official abbreviations
   
   ABW	Aruba
   AFG	Afghanistan
   AFS	South Africa
   AGL	Angola
   AIA	Anguilla
   ALB	Albania
   ALG	Algeria
   ALS	Alaska *
   AMS	Saint Paul & Amsterdam Is.
   AND	Andorra
   AOE  Western Sahara
   ARG	Argentina
   ARM	Armenia
   ARS	Saudi Arabia
   ASC	Ascension Island *
   ATA	Antarctica *
   ATG	Antigua and Barbuda
   ATN	Netherlands Leeward Antilles (dissolved in 2010)
   AUS	Australia
   AUT	Austria
   AZE	Azerbaijan
   AZR	Azores
   B	Brasil
   BAH	Bahamas
   BDI	Burundi
   BEL	Belgium
   BEN	Benin
   BER	Bermuda
   BES  Bonaire, St Eustatius, Saba (Dutch islands in the Caribbean)
   BFA	Burkina Faso
   BGD	Bangla Desh
   BGR  Bulgaria
   BHR	Bahrain
   BIH	Bosnia-Herzegovina
   BIO	Chagos Is. (Diego Garcia) (British Indian Ocean Territory)
   BLM  Saint-Barthelemy
   BLR	Belarus
   BLZ	Belize
   BOL	Bolivia
   BOT	Botswana
   BOV	Bouvet *
   BRB	Barbados
   BRU	Brunei Darussalam
   BTN	Bhutan
   CAB	Cabinda *
   CAF	Central African Republic
   CAN	Canada
   CBG	Cambodia
   CEU	Ceuta *
   CHL	Chile
   CHN	China (People's Republic)
   CHR	Christmas Island (Indian Ocean)
   CKH	Cook Island (South) (Rarotonga)
   CLA  Clandestine stations *
   CLM	Colombia
   CLN	Sri Lanka
   CLP	Clipperton *
   CME	Cameroon
   CNR	Canary Islands
   COD  Democratic Republic of Congo (capital Kinshasa)
   COG  Republic of Congo (capital Brazzaville)
   COM	Comores
   CPV	Cape Verde Islands
   CRO	Crozet Island
   CTI	Ivory Coast (Côte d'Ivoire)
   CTR	Costa Rica
   CUB	Cuba
   CUW  Curacao
   CVA	Vatican State
   CYM	Cayman Islands
   CYP	Cyprus
   CZE	Czech Republic
   D	Germany
   DJI	Djibouti
   DMA	Dominica
   DNK	Denmark
   DOM	Dominican Republic
   E	Spain
   EGY	Egypt
   EQA	Ecuador
   ERI	Eritrea
   EST	Estonia
   ETH	Ethiopia
   EUR	Iles Europe & Bassas da India *
   F	France
   FIN	Finland
   FJI	Fiji
   FLK	Falkland Islands
   FRO	Faroe Islands
   FSM	Federated States of Micronesia
   G	Great Britain and Northern Ireland
   GAB	Gabon
   GEO	Georgia
   GHA	Ghana
   GIB	Gibraltar
   GLP	Guadeloupe
   GMB	Gambia
   GNB	Guinea-Bissau
   GNE	Equatorial Guinea
   GPG	Galapagos *
   GRC	Greece
   GRD	Grenada
   GRL	Greenland
   GTB	Guantanamo Bay *
   GTM	Guatemala
   GUF	French Guyana
   GUI	Guinea
   GUM	Guam / Guahan
   GUY	Guyana
   HKG	Hong Kong, part of China
   HMD	Heard & McDonald Islands *
   HND	Honduras
   HNG	Hungary
   HOL	The Netherlands * (changed to NLD)
   HRV	Croatia
   HTI	Haiti
   HWA	Hawaii *
   HWL	Howland & Baker
   I	Italy
   ICO	Cocos Island
   IND	India
   INS	Indonesia
   IRL	Ireland
   IRN	Iran
   IRQ	Iraq
   ISL	Iceland
   ISR	Israel
   IW   International Waters
   IWA	Ogasawara (Bonin, Iwo Jima) *
   J	Japan
   JAF	Jarvis Island
   JDN	Juan de Nova *
   JMC	Jamaica
   JMY	Jan Mayen *
   JON	Johnston Atoll
   JOR	Jordan
   JUF	Juan Fernandez Island *
   KAL	Kaliningrad *
   KAZ	Kazakstan / Kazakhstan
   KEN	Kenya
   KER	Kerguelen
   KGZ	Kyrgyzstan
   KIR	Kiribati
   KNA  St Kitts and Nevis
   KOR	Korea, South (Republic)
   KRE	Korea, North (Democratic People's Republic)
   KWT	Kuwait
   LAO	Laos
   LBN	Lebanon
   LBR	Liberia
   LBY	Libya
   LCA	Saint Lucia
   LIE	Liechtenstein
   LSO	Lesotho
   LTU	Lithuania
   LUX	Luxembourg
   LVA	Latvia
   MAC	Macao
   MAF  St Martin
   MAU	Mauritius
   MCO	Monaco
   MDA	Moldova
   MDG	Madagascar
   MDR	Madeira
   MDW  Midway Islands
   MEL	Melilla *
   MEX	Mexico
   MHL  Marshall Islands
   MKD	Macedonia (F.Y.R.)
   MLA	Malaysia
   MLD	Maldives
   MLI	Mali
   MLT	Malta
   MMR  Myanmar (Burma)
   MNE  Montenegro
   MNG	Mongolia
   MOZ	Mozambique
   MRA	Northern Mariana Islands
   MRC	Morocco
   MRN	Marion & Prince Edward Islands
   MRQ	Marquesas Islands *
   MRT	Martinique
   MSR	Montserrat
   MTN	Mauritania
   MWI	Malawi
   MYT	Mayotte
   NCG	Nicaragua
   NCL	New Caledonia
   NFK	Norfolk Island
   NGR	Niger
   NIG	Nigeria
   NIU	Niue
   NLD  Netherlands
   NMB	Namibia
   NOR	Norway
   NPL	Nepal
   NRU	Nauru
   NZL	New Zealand
   OCE	French Polynesia
   OMA	Oman
   PAK	Pakistan
   PAQ	Easter Island
   PHL	Philippines
   PHX	Phoenix Is.
   PLM	Palmyra Island
   PLW  Palau
   PNG	Papua New Guinea
   PNR	Panama
   POL	Poland
   POR	Portugal
   PRG	Paraguay
   PRU	Peru
   PRV	Okino-Tori-Shima (Parece Vela) *
   PSE  Palestine
   PTC	Pitcairn
   PTR	Puerto Rico
   QAT	Qatar
   REU	La Réunion
   ROD  Rodrigues
   ROU	Romania
   RRW	Rwanda
   RUS	Russian Federation
   S	Sweden
   SAP	San Andres & Providencia *
   SDN	Sudan
   SEN	Senegal
   SEY	Seychelles
   SGA	South Georgia Islands *
   SHN	Saint Helena
   SLM	Solomon Islands
   SLV	El Salvador
   SMA	Samoa (American)
   SMO	Samoa
   SMR	San Marino
   SNG	Singapore
   SOK	South Orkney Islands *
   SOM	Somalia
   SPM	Saint Pierre et Miquelon
   SPR	Spratly Islands *
   SRB  Serbia
   SRL	Sierra Leone
   SSI	South Sandwich Islands *
   STP	Sao Tome & Principe
   SUI	Switzerland
   SUR	Suriname
   SVB	Svalbard *
   SVK	Slovakia
   SVN	Slovenia
   SWZ	Swaziland
   SXM  Sint Maarten
   SYR	Syria
   TCA	Turks and Caicos Islands
   TCD	Tchad
   TGO	Togo
   THA	Thailand
   TJK	Tajikistan
   TKL	Tokelau
   TKM	Turkmenistan
   TLS  Timor-Leste
   TON	Tonga
   TRC	Tristan da Cunha *
   TRD	Trinidad and Tobago
   TUA	Tuamotu Archipelago *
   TUN	Tunisia
   TUR	Turkey
   TUV	Tuvalu
   TWN  Taiwan *
   TZA	Tanzania
   UAE	United Arab Emirates
   UGA	Uganda
   UKR	Ukraine
   UN   United Nations *
   URG	Uruguay
   USA	United States of America
   UZB	Uzbekistan
   VCT	Saint Vincent and the Grenadines
   VEN	Venezuela
   VIR	American Virgin Islands
   VRG	British Virgin Islands
   VTN	Vietnam
   VUT	Vanuatu
   WAK	Wake Island
   WAL	Wallis and Futuna
   XUU  Unidentified
   YEM	Yemen
   ZMB	Zambia
   ZWE	Zimbabwe

   III) Target-area codes.
   Af  - Africa
   Am  - America(s)
   As  - Asia
   C.. - Central ..
   Car - Caribbean
   Cau - Caucasus
   CIS - Commonwealth of Independent States (Former Soviet Union)
   CNA - Central North America
   E.. - East ..
   ENA - Eastern North America
   Eu  - Europe (often including North Africa/Middle East)
   FE  - Far East
   LAm - Latin America (=Central and South America)
   ME  - Middle East
   N.. - North ..
   NAO - North Atlantic Ocean
   Oc  - Oceania (Australia, New Zealand, Pacific Ocean)
   S.. - South ..
   SAO - South Atlantic Ocean
   SEA - South East Asia
   SEE - South East Europe
   Sib - Siberia
   Tib - Tibet
   W.. - West ..
   WNA - Western North America
   
   Alternatively, ITU country codes may be used if the target area is limited to
   a certain country. This is often the case for domestic stations.
   
   IV) Transmitter site codes.
   One-letter or two-letter codes for different transmitter sites within one country.
   No such code is used if there is only one transmitter site in that country.
   
   The code is used plainly if the transmitter is located within the home country of the station.
   Otherwise, it is preced by "/ABC" where ABC is the ITU code of the host country of the transmitter site.
   Example: A BBC broadcast, relayed by the transmitters in Samara, Russia, would be designated as "/RUS-s".
   
   In many cases, a station has a "major" transmitter site which is normally not designated. Should this station
   use a different transmitter site in certain cases, they are marked.
   No transmitter-site code is used when the transmitter site is not known.
   
   Resources for transmitter sites include:
    For Brazil: http://sistemas.anatel.gov.br/siscom/consplanobasico/default.asp
    For USA and territories: http://transition.fcc.gov/ib/sand/neg/hf_web/stations.html
    Google Earth and Wikimapia
   Many more details can be discussed in an online group dedicated to BC SW transmitter sites: http://groups.yahoo.com/group/shortwavesites/
    
   
   AFG: k-Kabul / Pol-e-Charkhi 34N35-69E12
        x-Khost 33N14-69E49
        y-Kabul-Yakatut 34N32-69E13
   AFS: Meyerton 26S35-28E08 except:
        ct-Cape Town 33S41-18E42
   AGL: Mulenvos 08S53-13E20
        L-Luena (Moxico) 11S47'00"-19E55'19"
   AIA: The Valley 18N13-63W01
   ALB: Cerrik (CRI) 41N00-20E00 except:
        f-Fllake (Durres, 500kW) 41N22-19E30
        s-Shijiak (Radio Tirana) (1x100kW = 2x50kW) 41N21-19E35
   ALG: b-Bechar
        o-Ourgla 31N55-05E04
        t-Tindouf 27N30-07W50
   ALS: ap-Anchor Point 59N44'58"-151W43'56"
        e-Elmendorf AFB 61N15'04"-149W48'23"
        g-Gakona 62N23'30"-145W08'48"
        k-Kodiak 57N46'30"-152W33'41"
   ARG: b-Buenos Aires 34S37'19"-58W21'18"
        g-General Pacheco 34S36-58W22
   ARM: Gavar (formerly Kamo) 40N25-45E12
        y-Yerevan 40N10-44E30
   ARS: Riyadh 24N30-46E23 except:
        j-Jeddah 21N23-39E10
   ASC: Ascension Island, 07S54-14W23
   ATA: Base Esperanza 63S24-57W00
   AUS: a-Alice Springs NT 23S49-133E51
        b-Brandon QL 19S31-147E20
        h-Humpty Doo NT 12S34-131E05
        ka-Katherine NT 14S24-132E11
        ku-Kununurra WA 15S48-128E41
        L-Sydney-Leppington NSW
        n-Ningi QL 27S04'00"-153E03'20"
        s-Shepparton VIC 36S20-145E25
        sf-Shofields, western Sydney
        sm-St Mary's, Sydney 33S45-150E46
        t-Tennant Creek NT 19S40-134E16
   AUT: Moosbrunn 48N00-16E28
   AZE: g-Gäncä 40N36-46E20
        s-Stepanakert 39N49'35"-46E44'23"
   B:   a-Porto Alegre, RS 30S03-51W23
        ag-Araguaína, TO 07S12-48W12
        an-Anápolis, GO 16S15'25"-49W01'08"
        ap-Aparecida, SP 22S50'47"-45W13'13"
        ar-Araraquara, SP 21S48-48W11
        b-Brasilia, Parque do Rodeador, DF 15S36'40"-48W07'53"
        be-Belém, PA 01S27-48W29
        br-Braganca, PA 01S03'48"-46W46'24"
        bt-Belém, PA (Ondas Tropicais 5045) 01S22-48W21
        bv-Boa Vista, RR 02S49-60W40
        c-Contagem/Belo Horizonte, MG 19S53'59"-44W03'16"
        ca-Campo Largo (Curitiba), PR 25S25'48"-49W23'49"
        cb-Camboriú, SC 27S01'31"-48W39'13"
        cc-Cáceres, MT 16S04'36"-57W38'27"
        cg-Campo Grande, MS 20S31'12"-54W35'00"
        Cg-Campo Grande, MS 20S27-54W37
        cm-Campinas, SP 22S56'52"-47W01'05"
        cn-Congonhas, MG 20S30-43W52
        co-Coari, AM 04S06'59"-63W07'31"
        cp-Cachoeira Paulista, SP 22S38'45"-45W04'42"
        Cp-Cachoeira Paulista, SP 22S38'39"-45W04'38"
        cs-Cruzeiro do Sul, Estrada do Aeroporto, AC 07S38-72W40
        cu-Curitiba, PR 25S27'08"-49W06'50"
        E-Esteio (Porto Alegre), RS 29S49'41"-51W09'54"
        e-Esteio (Porto Alegre), RS 29S51'59"-51W06'11"
        f-Foz do Iguacu, PR 25S31'03"-54W30'30"
        fl-Florianópolis, SC 27S36'09"-48W31'51"
        fp-Florianópolis, SC 27S35'14"-48W32'17"
        g-Guarujá, SP 23S59'35"-46W15'23"
        gb-Guaíba (Porto Alegre), RS 29S59'50"-51W17'08"
        gc-Sao Gabriel de Cachoeira, AM 00S08-67W05
        gm-Guajará-Mirim, RO 10S47-65W20
        go-Goiânia, GO 16S40-49W15
        Go-Goiânia, 16S39'27"-49W14'06"
        gu-Guarulhos, SP 23S26-46W25
        h-Belo Horizonte, MG 19S58'34"-43W56'00"
        ib-Ibitinga, SP 21S46'20"-48W50'10"
        ld-Londrina, PR 23S20'16"-51W13'18"
        li-Limeira, SP 22S33'39"-47W25'08"
        lo-Londrina, PR 23S24'17"-51W09'19"
        m-Manaus AM 03S06-60W02
        ma-Manaus - Radiodif.Amazonas, AM 03S08'28"-59W59'30"
        mc-Macapá, AP 00N03'50"-51W02'20"
        mg-Manaus - Radio Globo, AM 03S08'04"-59W58'39"
        mi-Marília, SP 22S13'33"-49W57'46"
        ob-Óbidos, PA 01S55-55W31
        os-Osasco, SP 23S30'51"-46W35'39"
        pa-Parintins, AM 02S37-56W45
        pc-Pocos da Caldas, MG 21S47'52"-46W32'26"
        pe-Petrolina, PE 09S24-40W30
        pi-Piraquara (Curitiba), PR 25S23'34"-49W10'04"
        pv-Porto Velho, RO 08S45-63W55 (4 dipolos em quadrado)
        r-Rio de Janeiro (Radio Globo), RJ 22S49'24"-43W05'49"
        rb-Rio Branco, AC 09S58-67W49
        rc-Rio de Janeiro (Radio Capital), RJ 22S46'43"-43W00'56"
        rj-Rio de Janeiro (Radio Relogio), RJ 22S46'41"-42W59'02"
        ro-Rio de Janeiro, Observatório Nacional, 22S53'45"-43W13'27"
        rs-Rio de Janeiro (Super Radio), RJ 22S49'22"-43W05'21"
        rw-Rio de Janeiro PWZ 22S57-42W55
        sa-Santarém, PA 02S26'55"-54W43'58"
        sb-Sao Paulo - Radio Bandeirantes, SP 23S38'54"-46W36'02"
        sc-Sao Paulo - Radio Cultura, SP 23S30'42"-46W33'41"
        sg-Sao Paulo - Radio Globo, SP 23S36'26"-46W26'12"
        sj-Sao Paulo - Radio 9 de Julho, SP 23S32'51"-46W38'10"
        sm-Santa Maria, RS 29S44'18"-53W33'19"
        sr-Sao Paulo - Radio Record, SP 23S41'02"-46W44'35"
        sz-Sao Paulo - Radio Gazeta, SP 23S40'10"-46W45'00"
        ta-Taubaté, SP 23S01-45W34
        te-Teresina, PI 05S05'13"-42W45'39"
        tf-Tefé, AM 03S21'15"-64W42'41"
        vi-Vitória, ES 20S19-40W19
        x-Xapuri, AL 10S39-68W30
   BEN: p-Parakou 09N20-02E38
   BES: Bonaire 12N12-68W18
   BFA: Ouagadougou 12N26-01W33
   BGD: Dhaka-Khabirpur 23N43-90E26
   BHR: Abu Hayan 26N02-50E37
   BIH: Bijeljina 44N42-19E10
   BIO: Diego Garcia 07S26-72E26
   BLR: Minsk-Sasnovy/Kalodziscy 53N58-27E47 except:
        b-Brest 52N18-23E54
        g-Hrodna/Grodno 53N40-23E50
        m-Mahiliou/Mogilev ("Orsha") 53N37-30E20
   BOL: ay-Santa Ana del Yacuma 13S45-65W32
        cb-Cochabamba 17S23-66W11
        ri-Riberalta 11S00-66W03
        sc-Santa Cruz 17S48-63W10
        uy-Uyuni 20S28-66W49
        yu-Yura 20S04-66W08
   BOT: Mopeng Hill 21S57-27E39
   BTN: Thimphu 27N28-89E39
   BUL: p-Plovdiv-Padarsko 42N23-24E52
        pe-Petrich 41N28-23E20
        s-Sofia-Kostinbrod 42N49-23E13
        vi-Vidin 43N50-22E43
   CAF: ba-Bangui 04N21-18E35
        bo-Boali 04N39-18E12
   CAN: Sackville 45N52'35"-64W19'29" except
        c-Calgary AB 50N54'02"-113W52'33"
        cc-Churchill MB 58N45'42"-93W56'39"
        ch-Coral Harbour NU 64N08'58"-83W22'22"
        cr-Cap des Rosiers 48N51'40"-64W12'53"
        cw-Cartwright NL 53N42'30"-57W01'17" and Hopedale NL 55N27'24"-60W12'30"
        g-Gander NL 48N57-54W37
        h-Halifax NS 44N41'03"-63W36'35"
        hx-Halifax CFH NS 44N57'50"-63W58'55"
        i-Iqaluit NU 63N43'42"-68W33'00"
        in-Inuvik NWT 68N19'33"-133W35'53"
        j-St John's NL 47N34'10"-52W48'52"
        k-Killinek NU 60N25'27"-64W50'30"
        o-Ottawa ON 45N17'41"-75W45'29"
        pc-Port Caledonia NS 46N11'14"-59W53'59"
        r-Resolute NU 74N45'14"-94W58'09"
        sa-St Anthony NL 51N30'00"-55W49'26"
        sj-St John's NL 47N36'40"-52W40'01"
        sl-St Lawrence NL 46N55'09"-55W22'45"
        sv-Stephenville NL 48N33'17"-58W45'32"
        t-Toronto (Aurora) ON 43N30'23"-79W38'01"
        tr-Trenton 43N50'39"-77W08'47"
        v-Vancouver BC 49N08'21"-123W11'44"
        ym-Yarmouth NB 43N44'39"-66W07'21"
   CGO: Brazzaville-M'Pila 04S15-15E18
   CHL: Santiago (Calera de Tango) 33S38-70W46 except:
        e-Radio Esperanza 38S41-72W35
        v-Valparaiso 32S48-71W28
   CHN: a-Baoji-Xinjie "722" 34N30-107E10
        as-Baoji-Sifangshan "724" 37N27-107E41
        b-Beijing-Matoucun "572" (CRI/CNR) 39N45-116E49
        B-Beijing-Gaobeidian "491" (CNR) 39N53-116E34
        bm-Beijing BAF 39N54-116E28
        bs-Beijing 3SD 39N42-115E55
        c-Chengdu (Sichuan) 30N54-104E07
        cc-Changchun (Jilin, 1017 kHz) 44N02-125E25
        ch-Changzhou Henglinchen (Jiangsu) 31N42-120E07
        d-Dongfang (Hainan) 18N53-108E39
        e-Gejiu (Yunnan) 23N21-103E08
        f-Fuzhou (Fujian) 26N01-119E17
        g-Gannan (Hezuo) 35N06-102E54
        gu-Gutian-Xincheng 26N34-118E44
        h-Hohhot (Nei Menggu, CRI) 40N48-111E47
        ha-Hailar (Nei Menggu) 49N11-119E43
        he-Hezuo 34N58-102E55
        hh-Hohhot-Yijianfang (Nei Menggu, PBS NM) 40N43-111E33
        j-Jinhua 29N07-119E19
        k-Kunming CRI (Yunnan) 24N53-102E30
        ka-Kashi (Kashgar) (Xinjiang) 39N21-75E46
        kl-Kunming-Lantao PBS (Yunnan) 25N10-102E50
        L-Lingshi (Shanxi) 36N52-111E40
        n-Nanning (Guangxi) 22N47-108E11
        nj-Nanjing (Jiangsu) 32N02-118E44
        p-Pucheng (Shaanxi) 35N00-109E31
        q-Ge'ermu/Golmud (Qinghai) 36N26-95E00
        qq-Qiqihar 47N02-124E03 (500kW)
        s-Shijiazhuang (Hebei) 38N13-114E06
        sg-Shanghai-Taopuzhen 31N15-121E29
        t-Tibet (Lhasa-Baiding) 29N39-91E15
        u-Urumqi (Xinjiang, CRI) 44N08'47"-86E53'43"
        uc-Urumqi-Changji (Xinjiang, PBS XJ) 43N58'26"-87E14'56"
        x-Xian-Xianyang "594" (Shaanxi) 34N12-108E54
        xc-Xichang (Sichuan) 27N49-102E14
        xg-Xining (Qinghai) 36N39-101E35
        xt-Xiangtan (Hunan) 27N30-112E30
   CLN: e-Ekala (SLBC,RJ) 07N06-79E54
        i-Iranawila (IBB) 07N31-79E48
        t-Trincomalee (DW) 08N44-81E10
   CME: Buea 04N09-09E14
   COD: bk-Bukavu 02S30-28E50
        bn-Bunia 01N32-30E11
   CTR: Cariari de Pococí (REE) 10N25-83W43 except:
        g-Guápiles (Canton de Pococí, Prov.de Limón) ELCOR
   CUB: La Habana Quivicán/Bejucal/Bauta 23N00-82W30
   CVA: Santa Maria di Galeria 42N03-12E19 except:
        v-Citta del Vaticano 41N54-12E27
   CYP: Zygi (Limassol) 34N43-33E19 except:
        g-Cape Greco 34N57-34E05
        y-Yeni Iskele 35N17-33E55
   D:   b-Biblis 49N41'18"-08E29'32"
        be-Berlin-Britz 52N27-13E26
        br-Braunschweig 52N17-10E43
        d-Dillberg 49N19-11E23
        dd-Dresden-Wilsdruff 51N03'31"-13E30'27"
        e-Erlangen-Tennenlohe 49N35-11E00
        g-Goehren 53N32'08"-11E36'40"
        ha-Hannover 52N23-09E42
        jr-Juliusruh 54N37'45"-13E22'26"
        k-Kall-Krekel 50N28'41"-06E31'23"
        L-Lampertheim 49N36'17"-08E32'20"
        n-Nauen 52N38'55"-12E54'33"
        nu-Nuernberg 49N27-11E05
        or-Oranienburg 52N47-13E23
        pi-Pinneberg 53N40'23"-09E48'30"
        r-Rohrbach 48N36-11E33
        w-Wertachtal 48N05'13"-10E41'42"
        wb-Wachenbrunn 50N29'08"-10E33'30"
        we-Weenermoor 53N12-07E19
   DJI: Djibouti 11N30-43E00
   DNK: co-Copenhagen OXT 55N50-11E25
   E:   Noblejas 39N57-03W26 except:
        c-Coruna 43N22'01"-08W27'07"
   EGY: a-Abis 31N10-30E05
        z-Abu Zaabal 30N16-31E22
   EQA: a-Ambato 01S13-78W37
        c-Pico Pichincha 00S11-78W32
        g-Guayaquil 02S16-79W54
        i-Ibarra 00N21-78W08
        o-Otavalo 00N18-78W11
        p-Pifo 00S14-78W20
        s-Saraguro 03S42-79W18
        t-Tena 01S00-77W48
        u-Sucúa 02S28-78W10
   ERI: Asmara-Saladaro 15N13-38E52
   ETH: a-Addis Abeba 09N00-38E45
        d-Geja Dera (HS) 08N46-38E40
        j-Geja Jewe (FS) 08N43-38E38
        m-Mekele 13N29-39E29
   F:   Issoudun 46N56-01E54 except:
        g-La Garde 43N06'16"-05E59'29"
        r-Rennes 48N06-01W41
   FIN: o-Ovaniemi 60N10'49"-24E49'35"
        p-Pori 61N28-21E35
        t-Topeno, Loppi, near Riihimäki 60N46-24E17
        v-Virrat 62N23-23E37
   FSM: Pohnpei 06N58-158E12
   G:   ab-Aberdeen 57N07'39"-02W03'13"
        cp-London-Crystal Palace 51N25-00W05
        cr-London-Croydon 51N25-00W05
        ct-Croughton 51N59'50"-01W12'33"
        d-Droitwich 52N18-02W06
        fm-Falmouth 49N57'37"-05W12'06"
        hu-Humber 54N07'00"-00W04'41"
        nw-Northwood 51N30-00W10
        o-Orfordness 52N06-01E35
        r-Rampisham 50N48'30"-02W38'35"
        s-Skelton 54N44-02W54
        sc-Shetland CG 60N24'06"-01W13'27"
        st-Stornoway 58N30'55"-06W15'40"
        w-Woofferton 52N19-02W43
   GAB: Moyabi 01S40-13E31
   GEO: s-Sukhumi 43N00-41E00
   GHA: Accra 05N31-00W10
   GNE: b-Bata 01N46-09E46
        m-Malabo 03N45-08E47
   GRC: a-Avlis 38N23-23E36
        i-Iraklion 35N20-25E09
        k-Kerkyra 39N37-19E55
        L-Limnos (Myrina) 39N52-25E03
        o-Olimpia 37N36'27"-21E29'15"
        r-Rhodos 36N27-28E13
   GRL: Tasiilaq
   GUF: Montsinery 04N54-52W29
   GUI: Conakry 09N32-13W40
   GUM: a-Station KSDA, Agta, 13N20'28"-144E38'56"
        b-Barrigada 13N29-144E50
        m-Station KTWR, Agana/Merizo 13N16'38"-144E40'16"
   GUY: Sparendaam 06N49-58W05
   HKG: a-Hongkong Airport 22N18-113E55
   HRV: Deanovec 45N39-16E27
   HWA: a-WWVH 21N59'21"-159W45'52"
        b-WWVH 21N59'11"-159W45'45"
        c-WWVH 21N59'18"-159W45'51"
        d-WWVH 21N59'15"-159W45'50"
        hi-Hickam AFB 21N19-157W55
        ho-Honolulu 21N19'32"-157W59'31"
        k-KVM Honolulu 21N20-158W00
        n-Naalehu 19N01-155W40
        nm-NMO Honolulu 21N26-158W09
        p-Pearl Harbour 21N25'41"-158W09'11"
   I:   a-Andrate 45N31-07E53
        an-Ancona IPA 
        b-San Benedetto de Tronto IQP
        ca-Cagliari IDC 
        cv-Civitavecchia IPD
        g-Genova ICB 
        L-Livorno IPL 
        p-Padova
        pt-Porto Torres IZN
        r-Roma
        ra-Roma IMB 41N47-12E28
        t-Trieste IQX
   IND: a-Aligarh (4x250kW) 28N00-78E06
        az-Aizawl(10kW) 23N43-92E43
        b-Bengaluru-Doddaballapur (Bangalore) 13N14-77E30
        bh-Bhopal(50kW) 23N10-77E30
        c-Chennai (Madras) 13N08-80E07
        d-Delhi (Kingsway) 28N43-77E12
        g-Gorakhpur 26N52-83E28
        gt-Gangtok 27N22-88E37
        hy-Hyderabad 17N20-78E33
        im-Imphal 24N37-93E54
        it-Itanagar 27N04-93E36
        j-Jalandhar 31N19-75E18
        ja-Jaipur 26N54-75E45
        je-Jeypore 18N55-82E34
        jm-Jammu 32N45-75E00
        k-Kham Pur, Delhi 110036 (Khampur) 28N49-77E08
        kh-Kohima 25N39-94E06
        ko-Kolkata(Calcutta)-Chinsurah 22N27-88E18
        ku-Kurseong 26N55-88E19
        le-Leh 34N08-77E29
        lu-Lucknow 26N53-81E03
        m-Mumbai (Bombay) 19N11-72E49
        n-Nagpur, Maharashtra 20N54-78E59
        p-Panaji (Goa) 15N31-73E52
        pb-Port Blair-Brookshabad 11N37-92E45
        r-Rajkot 22N22-70E41
        ra-Ranchi 23N24-85E22
        sg-Shillong 25N26-91E49
        sm-Shimla 31N06-77E09
        sr-Srinagar 34N00-74E50
        t-Tuticorin 08N48-78E12
        tv-Thiruvananthapuram(Trivendrum) 08N29-76E59
        w-Guwahati (1x200kW, 1x50kW) 26N11-91E50
   INS: bi-Biak, Papua 01S00-135E30
        bu-Bukittinggi, Sumatera Barat 00S18-100E22
        f-Fakfak, Irian Jaya Barat 02S55-132E17
        g-Gorontalo, Sulawesi 00N34-123E04
        j-Jakarta (Cimanggis) 06S12-106E51
        jm-Jambi, Sumatera 01S38-103E34
        kd-Kendari, Sulawesi Tenggara 03S58-122E30
        md-Manado, Sulawesi Utara 01N12-124E54
        mk-Manokwari, Irian Jaya Barat 00S48-134E00
        mr-Merauke, Papua 08S33-140E27
        ms-Makassar, Sulawesi Selatan 05S10-119E25
        n-Nabire, Papua 03S15-135E36
        p-Palangkaraya, Kalimantan Tengah 00S27-117E10
        pa-Palu, Sulawesi Tengah 00S36-129E36
        pd-Padang, Sumatera Barat 00S06-100E21
        pk-Pontianak 00S05-109E16
        se-Serui, Japen, Papua 01S48-136E26
        so-Sorong, Papua 00S52-131E25
        t-Ternate, Maluku Utara 00N48-127E23
        w-Wamena, Papua 03S48-139E53
   IRL: s-Shannon 52N42'32"-08W54'47"
   IRN: a-Ahwaz 31N20-48E40
        b-Bandar-e Torkeman 36N54-54E04
        bb-Bonab 37N18-46E03
        c-Chah Bahar 25N29-60E32
        k-Kamalabad 35N46-51E27
        ke-Kerman 29N59-56E46
        ki-Kiashar 37N24-50E01
        m-Mashhad 36N15-59E33
        mh-Mahidasht 34N16-46E48
        q-Qasr Shirin 34N27-45E37
        s-Sirjan 29N27-55E41
        t-Tayebad 34N44-60E48
        z-Zahedan 29N28-60E53
        zb-Zabol 31N02-61E33
   IRQ: d-Salah al-Din (Saladin) 34N27-43E35
        s-Sulaimaniya 35N33-45E26
   ISR: Yavne 31N52-34E45 except:
        L-Lot (Galei Zahal)
   J:   Yamata 36N10-139E50 except:
        c-Chiba Nagara 35N28-140E13
        f-Chofu Campus, Tokyo 35N39-139E33
        k-Kagoshima JMH 31N19-130E31
        ky-Kyodo 36N11-139E51
        n-Nemuro 43N18-145E34
        yo-Yokota AFB 35N44'55"-139E20'55"
   JOR: Al Karanah / Qast Kherane 31N44-36E26
   KAZ: Almaty (Alma Ata, Qaraturyq) 43N39-77E56
        d-Dmitrievka 43N50-77E00
   KEN: ny-Nairobi 5YE 01S15-36E52
   KGZ: Bishkek 42N54-74E37
   KOR: c-Chuncheon 37N56-127E46
        g-Goyang,Gyeonggi-do / Kyonggi-do 37N36-126E51
        h-Hwasung/Hwaseong 37N13-126E47
        j-Jeju/Aewol HLAZ 33N29-126E23
        k-Kimjae 35N50-126E50
        o-Suwon-Osan 37N09-127E00
        s-Seoul HLKX 37N34-126E58
        sg-Seoul-Gangseo-gu 37N34-126E58
        t-Taedok 36N23-127E22
   KRE: c-Chongjin 41N47-129E50
        e-Hyesan 41N04-128E02
        h-Hamhung 39N56-127E39
        j-Haeju 38N01-125E43
        k-Kanggye 40N58-126E36
        p-Pyongyang 39N05-125E23
        s-Sariwon 38N05-125E08
        u-Kujang 40N05-126E05
        w-Wonsan 39N05-127E25
        y-Pyongsong 40N05-124E24
   KWT: Kabd/Sulaibiyah 29N16-47E53
   LAO: s-Sam Neua 20N16-104E04
        v-Vientiane 17N58-102E33
   LBR: e-Monrovia ELWA 06N14-10W42
        m-Monrovia Mamba Point 06N19-10W49
        s-Star Radio Monrovia 06N18-10W47
   LBY: Sabrata 32N54-13E11
   LTU: Sitkunai 55N02'37"-23E48'28"
        v-Vilnius
   LUX: j-Junglinster 49N43-06E15
        m-Marnach 50N03-06E05
   LVA: Ulbroka 56N56-24E17
   MCO: Mont Angel/Fontbonne 43N46-07E25
   MDA: Maiac near Grigoriopol 47N17-29E24
   MDG: Talata Volonondry 18S50-47E35 except:
        a-Ambohidrano/Sabotsy 18S55-47E32
        m-Mahajanga (WCBC) 15S43'38"-46E26'45"
   MEX: c-Cuauhtémoc, Mexico City 19N26-99W09
        e-Mexico City (Radio Educación) 19N16-99W03
        i-Iztacalco, Mexico City 19N23-98W57
        m-Merida 20N58-89W36
        s-San Luis Potosi 22N10-101W00
        p-Chiapas 17N00-92W00
        u-UNAM, Mexico City 19N23-99W10
   MLA: ka-Kajang 03N01-101E46 except:
        kk-Kota Kinabalu 06N12-116E14
        ku-Kuching-Stapok (closed 2011) 01N33-110E20
        s-Sibu 02N18-111E49
   MLI: Bamako 12N39-08W01
   MNG: a-Altay 46N19-096E15
        m-Moron/Mörön 49N37-100E10
        u-Ulaanbaatar-Khonkhor 47N55-107E00
   MRA: m-Marpi, Saipan (KFBS) 15N16-145E48
        s-Saipan/Agingan Point (IBB) 15N07-145E41
        t-Tinian (IBB) 15N03-145E36
   MRC: b-Briech (VoA/RL/RFE) 35N33-05W58
        n-Nador (RTM,Medi1) 34N58-02W55
   MTN: Nouakchott 18N07-15W57
   MYA: n-Naypyidaw 20N11-96E08
        p-Phin Oo Lwin, Mandalay 22N01-96E33
        t-Taunggyi(Kalaw) 20N38-96E35
        y-Yegu (Yangon/Rangoon) 16N52-96E10
   NGR: Niamey 13N30-02E06
   NIG: a-Abuja-Gwagwalada 08N56-07E04
        b-Ibadan 07N23-03E54
        e-Enugu 06N27-07E27
        i-Ikorodu 06N36-03E30
        j-Abuja-Lugbe (new site, opened March 2012) 08N58-07E21
        k-Kaduna 10N31-07E25
   NOR: Kvitsoy 59N08-05E15 except
        a-Andenes 69N16'08"-16E02'26"
        as-Andenes-Saura 69N08'24"-16E01'12"
   NPL: Khumaltar 27N30-85E30
   NZL: Rangitaiki 38S50-176E25
   OMA: Radio Oman: t-Thumrait 17N38-53E56
                    s-Seeb 23N40-58E10
        BBC: A'Seela 21N55-59E37
   PAK: Islamabad 33N27-73E12 except:
        p-Peshawar 34N00-71E30
        q-Quetta 30N15-67E00
        r-Rawalpindi 33N30-73E00
   PHL: b-Bocaue (FEBC) 14N48-120E55
        i-Iba (FEBC) 15N20-119E58
        m-Marulas/Quezon City, Valenzuela (PBS 6170,9581) 14N41-120E58
        p-Palauig, Zembales (RVA) 15N28-119E50
        po-Poro 16N26-120E17
        t-Tinang (VoA) 15N21-120E37
        x-Tinang-2/portable 50kW (VoA) 15N21-120E37
   PLW: Koror-Babeldaob (Medorn) 07N27'22"-134E28'24"
   PNG: a-Alotau 10S18-150S28
        d-Daru 09S05-143E10
        g-Goroka 06S02-145E22
        ka-Kavieng 02S34-150E48
        kb-Kimbe 05S33-150E09
        ke-Kerema 07S59-145E46
        ki-Kiunga 85 km south of Tabubil
        ku-Kundiawa 06S00-144E57
        la-Lae (Morobe) 06S44-147E00
        lo-Lorengau 02S01-147E15
        ma-Madang 05S14-145E45
        me-Mendi 06S13-143E39
        mh-Mount Hagen 05S54-144E13
        pm-Port Moresby (Waigani) 09S28-147E11
        po-Popondetta 08S45-148E15
        r-Rabaul 04S13-152E13
        v-Vanimo 02S40-141E17
        va-Vanimo 02S41-141E18
        wa-Port Moresby (Radio Wantok) 09S28-147E11
        ww-Wewak 03S35-143E40
   POR: Sines 37N57-08W46 except:
        g-Sao Gabriel(Lisboa) 38N47-08W42
   PRU: ar-Arequipa 16S25-71W32
        at-Atalaya 10S44-73W45
        bv-Bolívar 07S21-77W50
        cc-Chiclayo 06S46-79W50
        ce-Celendín 06S53-78W09
        ch-Chachapoyas 06S14-77W52
        cl-Callalli 15S30-71W26
        cp-Cerre de Pasco 10S40-76W15
        ct-Chota 06S33-78W39
        cu-Cuzco 13S32-71W57
        hb-Huancabamba 05S14-79W27
        hc-Huancayo 12S05-75W12
        ho-Huánuco 09S56-76W14
        hu-Huanta 12S57-74W15
        hv-Huancavelica 12S47-74W59
        hz-Huaraz 09S31-77W32
        iq-Iquitos 03S45-73W15
        ja-Jaén 05S45-78W51
        ju-Junín 11S10-76W00
        li-Lima 12S06-77W03
        or-La Oroya 11S32-75W54
        pc-Paucartambo 10S54-75W51
        pm-Puerto Maldonado 12S36-69W10
        qb-Quillabamba 12S52-72W41
        rm-Rodrigues de Mendoza 06S23-77W30
        sc-Santa Cruz (R Satelite) 06S41-79W02
        si-Sicuani 14S16-71W14
        ta-Tarma 11S25-75W41
        tc-Tacna 18S00-70W13
        yu-Yurimaguas 05S54-76W07
   PTR: Roosevelt Roads 18N23-67W11
        p-WIPR San Juan 18N25'36"-66W08'29"
   ROU: g-Galbeni 46N45-26E41
        s-Saftica 100kW 44N38-27E05
        t-Tiganesti 300kW 44N45-26E05
   RRW: Kigali 01S58-30E04
   RUS: a-Armavir/Tblisskaya/Krasnodar 45N29-40E07
        ar-Arkhangelsk 64N22-41E24
        b-Blagoveshchensk (Amur) 50N16-127E33
        c-Chita (Atamanovka) (S Siberia) 51N50-113E43
        gr-Grozny
        i-Irkutsk (Angarsk) (S Siberia) 52N25-103E40
        k-Kaliningrad-Bolshakovo 54N54-21E43
        ka-Komsomolsk-na-Amur (Far East) 50N39-136E55
        kh-Khabarovsk (Far East) 48N33-135E15
        kr-Krasnoyarsk 56N02-92E44
        ku-Kurovskaya-Avsyunino (near Moscow) 55N34-39E09
        ky-Kyzyl 51N41-94E36
        L-Lesnoy (near Moscow) 56N04-37E58
        m-Moscow/Moskva (one of ku,L,se,t) 55N45-37E18 
        ma-Magadan 59N42-150E10
        mr-Moscow-Razdory 55N45-37E18
        mu-Murmansk/Monchegorsk 67N55-32E59
        n-Novosibirsk / Oyash, (500 kW, 1000 kW) 55N31-83E45
        no-Novosibirsk City (100 kW, 200 kW) 55N04-82E58
        p-Petropavlovsk-Kamchatskij (Yelizovo) 52N59-158E39
        pe-Perm 58N03-56E14
        s-Samara (Zhygulevsk) 53N17-50E15
        sp-St.Petersburg (Popovka/Krasnyj Bor) 59N39-30E42
        t-Taldom - Severnyj, Radiotsentr 3 (near Moscow) 56N44-37E38
        tv-Tavrichanka 43N20-131E54
        u-Ulan-Ude 51N44-107E26
        us-Ulan-Ude/Selenginsk 52N02-106E56
        v-Vladivostok Razdolnoye (Ussuriysk Tavrichanka) 43N32-131E57
        ya-Yakutsk/Tulagino 62N01-129E48
        ys-Yuzhno-Sakhalinsk (Vestochka) 46N55-142E54
   S:   Hoerby 55N49-13E44
        k-Kvarnberget-Vallentuna 59N30-18E08
        s-Sala 59N55-16E28
   SDN: a-Al-Aitahab 15N35-32E26
        n-Narus 04N46-33E35
        r-Reiba 13N33-33E31
   SEY: Mahe 04S36-55E28
   SLM: Honiara 09S25-160E03
   SNG: Kranji 01N25-103E44 except:
        j-Jurong 01N16-103E40
        v-Singapore Volmet 01N20-103E59
   SOM: h-Hargeisa 09N33-44E03
   SRB: s-Stubline 44N34-20E09
   STP: Pinheira 00N18-06E42
   SVK: Rimavska Sobota 48N24-20E08
   SWZ: Manzini/Mpangela Ranch 26S34-31E59
   SYR: Adra 33N27-36E30
   TCD: N'Djamena-Gredia 12N08-15E03
   THA: b-Bangkok / Prathum Thani 14N03-100E43
        bm-Bangkok Meteo 13N44-100E30
        bv-Bangkok Volmet 13N41'40"-100E46'14"
        n-Nakhon Sawan 15N49-100E04
        u-Udon Thani 17N25-102E48
   TJK: y-Yangi Yul (Dushanbe) 38N29-68E48
        o-Orzu 37N32-68E42
   TKM: Asgabat 37N51-58E22
   TUN: Sfax 34N48-10E53
   TUR: c-Cakirlar 39N58-32E40
        ca-Catalca (Istanbul) 
        e-Emirler 39N29-32E51
   TWN: f-Fangliao FAN 22N22-120E33
        h-Huwei (Yunlin province) 23N43-120E25
        k-Kouhu (Yunlin province) 23N32-120E10
        L-Lukang 24N03-120E25
        m-Minhsiung (Chiayi province) 23N33-120E25
        n-Tainan/Annan (Tainan city) 23N02-120E10
        p-Paochung/Baujong (Yunlin province) PAO/BAJ 23N43-120E18
        s-Tanshui (Taipei province) 25N11-121E25
        t-Taipei (Pali) 25N06-121E26
        y-Kuanyin (Han Sheng) 25N02-121E06
   TZA: d-Daressalam 06S50-39E14
        z-Zanzibar/Dole 06S05-39E14
   UAE: Dhabbaya 24N11-54E14
   UGA: k-Kampala-Bugolobi 00N20-32E36
        m-Mukono 00N21-32E46
   UKR: c-Chernivtsi 48N18-25E50
        k-Kiev/Brovary 50N31-30E46
        L-Lviv (Krasne) (Russian name: Lvov) 49N51-24E40
        m-Mykolaiv (Kopani) (Russian name:Nikolayev) 46N49-32E14
        x-Kharkiv (Taranivka) (Russian name: Kharkov) 49N38-36E07
        z-Zaporizhzhya 47N50-35E08
   URG: m-Montevideo 34S50-56W15
   USA: a-Andrews AFB 38N48'39"-76W52'01"
        b-Birmingham / Vandiver, AL (WEWN) 33N30'13"-86W28'27"
        bg-Barnegat, NJ 39N42-74W12
        bo-Boston, MA 41N42'30"-70W33'00"
        bt-Bethel, PA (WMLK) 40N28'46"-76W16'47"
        c-Cypress Creek, SC (WHRI) 32N41'03"-81W07'50"
        ch-Chesapeake, VA 36N33'34"-76W17'02"
        fa-Fort Collins, CO 40N40'55"-105W02'31"
        fb-Fort Collins, CO 40N40'42"-105W02'25"
        fc-Fort Collins, CO 40N40'48"-105W02'25"
        fd-Fort Collins, CO 40N40'45"-105W02'25"
        fe-Fort Collins, CO 40N40'53"-105W02'29"
        g-Greenville, NC 35N28-77W12
        k-Key Saddlebunch, FL 24N39-81W36
        L-Lebanon, TN (WTWW) 36N16'35"-86W05'58"
        m-Miami / Hialeah Gardens, FL (WRMI) 25N54'00"-80W21'49"
        ma-Manchester / Morrison, TN (WWRB) 35N37'27'-86W00'52"
        mi-Milton, FL (WJHR) 30N39'03"-87W05'27"
        mo-Mobile, AL (WLO)
        n-Nashville, TN (WWCR) 36N12'30"-86W53'38"
        nm-NMG New Orleans, LA 29N53'04"-89W56'43"
        no-New Orleans, LA (WRNO) 29N50'10"-90W06'57"
        np-Newport, NC (WTJC) 34N46'41"-76W52'37"
        o-Okeechobee, FL (WYFR) 27N27'30"-80W56'00"
        of-Offutt AFB, NE 41N06'49"-95W55'42"
        q-Monticello, ME (WBCQ) 46N20'30"-67W49'40"
        pr-Point Reyes, CA 37N55'32"-122W43'52"
        rl-Red Lion (York), PA (WINB) 39N54'22"-76W34'56"
        rs-Los Angeles / Rancho Simi, CA (KVOH) 34N15'23"-118W38'29"
        v-Vado, NM (KJES) 32N08'02"-106W35'24"
   UZB: Tashkent 41N13-69E09 except:
        a-Tashkent Airport 41N16-69E17
        ta-Tashkent I/II 41N19-69E15
   VEN: y-YVTO Caracas 10N30'13"-66W55'44"
   VTN: b-Buon Me Thuot, Daclac 12N38-108E01
        db-Dien Bien 21N22-103E00
        L-Son La 21N20-103E55
        m-Hanoi-Metri 21N00-105E47
        s-Hanoi-Sontay 21N12-105E22
        x-Xuan Mai 20N43-105E33
   VUT: Empten Lagoon 17S45-168E21
   YEM: a-Al Hiswah/Aden 12N50-45E02
        s-Sanaa 15N22-44E11
   ZMB: L-Lusaka 15S30-28E15
        m-Makeni Ranch 15S32-28E00
   ZWE: Gweru/Guinea Fowl 19S26-29E51

