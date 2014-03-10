#include <NewPing.h>
#include <MIDI.h>
#include <math.h>
#include <Line.h>

#define TRIGGER_PIN 7
#define ECHO_PIN 8
#define MAX_DISTANCE 120
#define MAX_SONAR_PING 4300
#define LED_PIN 11
#define PING_MEDIAN 25
#define SONAR_UNCERTAINCY 45
#define STEPS 4
#define SENSIBILITE 1

NewPing sonar = NewPing(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
char last_midi_note = 0;
char last_midi_pitch = 0;
int last_sonar_ping = 0;
int last_sonar_pings[PING_MEDIAN];
int i = 0;
char loop_number = 0;
int dernieresValeurs[10];
int derniersEcarts[10];
Line<int> interpolation;

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    
    // MIDI.begin();
    
    // Initalises last_pings to zero
    for(i=0; i<PING_MEDIAN; i++) {
        last_sonar_pings[i] = 0;
    }
    for(i=0; i<10; i++){
        dernieresValeurs[i]=0;
        derniersEcarts[i]=0;
    }
}

/**
 * Returns a round midi note
 * Use midi_pitch_round() to get the pitch correction of a precise frequency
 *
 * @param frequency The frequency (in Hz) to parse
 */
int midi_note_round(int frequency) {
    return 69 + 12*log(frequency/440.0)/log(2);
}

/**
 * Returns a midi pitch
 * Use midi_note_round() to get the midi base note to adjust
 * 
 * @param frequency The frequency (in Hz) to parse
 */
int midi_pitch_round(int frequency) {
    return floor((71 + 12 * log((frequency/440.0))/log(2) - floor(69 + 12 * log((frequency/440.0))/log(2))) * 32);
}

/**
 * Autotunes a note on a given scale
 * 
 * @param note the midi note
 * @param start the first note of the scale (default : C)
 */
int scale_round(int note, int start = 0) {
    
    int closest_note = -1;
    int octave = 0;
    
    int notes[] = {0, 2, 4, 6, 8, 10};
    int number_of_notes = 6;
    
    note -= start;
    octave = note/12;
    note %= 12;
    
    for(int i=0; i < number_of_notes && closest_note == -1; i++) {
        if (i == number_of_notes - 1 || note <= notes[i] + (notes[i+1] - notes[i])/2.0) {
            closest_note = notes[i];
        }
    }
    
    return closest_note + 12 * octave + start;
}


/**
 * @return the median ping of the last readings
 */
int read_sonar_ping() {
    int sonar_ping = sonar.ping_median(2);
    int ping_max = 0, ping_min = 0;
    
    if(sonar_ping > 0 && sonar_ping < MAX_SONAR_PING) {
        
        // Finds the median for the last PING_MEDIAN recorded values
        for(i=1; i<PING_MEDIAN && last_sonar_pings[i] != 0; i++) {
            
            ping_max = (ping_max == 0 || last_sonar_pings[i] > ping_max ? last_sonar_pings[i] : ping_max);
            ping_min = (ping_min == 0 || last_sonar_pings[i] < ping_min ? last_sonar_pings[i] : ping_min);
            
            // Moves cached pings one square to the left
            last_sonar_pings[i-1] = last_sonar_pings[i];
        }
        
        // Stocks the last sonar_ping in the first empty spot found in last_sonar_pings
        last_sonar_pings[i] = sonar_ping;
        
        // Checks if last_sonar_ping is outside the range of uncertaincy
        if(ping_max && ping_min &&
            (sonar_ping - SONAR_UNCERTAINCY > ping_max) || 
            (sonar_ping + SONAR_UNCERTAINCY < ping_min)) {
            // Serial.println(" -- Unstable -- ");
            
            // --> Unsable signal <--

            // Resets median
            // Flush old values when the captured distance becomes unstable
            for(i=0; i<PING_MEDIAN; i++) {
                last_sonar_pings[i] = 0;
            }
        }
        
        return (ping_max + ping_min)>>1; // '>>1' is just some fancy binary stuff for a divison by 2
    }
    
    return 0;
}

/**
 * @param tableau de int
 * @return moyenne des valeurs du tableau
 */
int moyenne(int valeurs[]) {
    int moyenne = 0;
    int longueurTableau = 0;
        for(int valeur: valeurs){
            moyenne += valeur;
            longueurTableau ++;
        }
        moyenne /= longueurTableau;
    return moyenne;
}

void ajoutValeur(int valeurs[], int nouvelleValeur)
{
    int longueur = sizeof(valeurs)/sizeof(valeurs[0]);
    for(int i=longueur-1; i>0; i++)
    {
        valeurs[i] = valeurs[i-1];
    }
    valeur[0]=nouvelleValeur;
}

boolean isValeurBonne()
{
    if(derniersEcarts[0]>SENSIBILITE*moyenne(derniersEcarts))
    {
        return true;
    }
    return false;
}

void loop() {

    ajoutValeur(dernieresValeurs,sonar.ping());
    ajoutValeur(derniersEcarts,math.fabs(dernieresValeurs[0]-moyenne(dernieresValeurs)));
    Serial.println(isVAleurBonne());
    
    // Serial.println(sonar.ping());
    
    int ping_median = read_sonar_ping();
    
    /*
    loop_number = loop_number == STEPS ? 0 : loop_number;
    
    if(loop_number == 0) {
        loop_number = 0;
        int ping = read_sonar_ping();
        interpolation.set(ping, STEPS);
    }
    
    ping_median = interpolation.next();*/
    
    if(ping_median) {
        int ping_freq = map(ping_median, 0, 4000, 1000, 220);
        
        int ping_midi = midi_note_round(ping_freq);
        int ping_pitch = midi_pitch_round(ping_freq);
        
        if(ping_midi != last_midi_note && ping_pitch != last_midi_pitch) {

            // MIDI.sendPitchBend(ping_pitch, 1);
            MIDI.sendNoteOn(ping_midi, 100, 1);
            MIDI.sendNoteOff(last_midi_note, 0, 1);   // Stop the last note
            
            last_midi_note = ping_midi;
            last_midi_pitch = ping_pitch;
        }
    }
    
    //loop_number++;
}

/*
if(sonar_ping) {
    sonar_ping /= 10;
    if(last_sonar_ping + SONAR_UNCERTAINCY < sonar_ping || last_sonar_ping - SONAR_UNCERTAINCY > sonar_ping) {
        /*Serial.print(sonar_ping);
        Serial.print(" -> ");
        Serial.println(map(sonar_ping, 0, 429, 120, 80));
        
        int ping_midi = note_round(map(sonar_ping, 0, 429, 80, 40));
        if(ping_midi != last_midi_note) {
            MIDI.sendNoteOn(ping_midi, 100, 1);
            MIDI.sendNoteOff(last_midi_note, 0, 1);   // Stop the note
            last_midi_note = ping_midi;
        }
    }
    
    last_sonar_ping = sonar_ping;
}
/*
if(abs(sonar_ping - last_sonar_ping) > SONAR_UNCERTAINCY) {
    last_sonar_ping = sonar_ping;
    if (sonar_ping > 0 && sonar_ping < 2500) {
        
        int ping_midi = note_round(map(sonar_ping, 0, 2500, 96, 80));
        if(ping_midi != last_midi_note) {
            MIDI.sendNoteOn(ping_midi, 100, 1);
            MIDI.sendNoteOff(last_midi_note, 0, 1);   // Stop the note
            last_midi_note = ping_midi;
        }
        
        int ping = map(sonar_ping, 0, 2500, 0, 255);

        ping = 255 - ping;
        analogWrite(LED_PIN, ping);
    }
}*/
