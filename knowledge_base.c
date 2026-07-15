#include "assistant.h"

/* ================================================================== */
/*  RUNTIME KNOWLEDGE BASE  — loaded from edubot_data.txt             */
/* ================================================================== */
static KnowledgeItem KB_STORE[MAX_KB_ENTRIES];
static int           KB_COUNT = 0;

/* Map topic string in dataset file to SubjectTopic enum */
static SubjectTopic parse_topic_str(const char *s) {
    if (strstr(s,"physics"))     return TOPIC_PHYSICS;
    if (strstr(s,"civil"))       return TOPIC_CIVIL_ENGINEERING;
    if (strstr(s,"maths")||strstr(s,"math")) return TOPIC_MATHS;
    if (strstr(s,"ai")||strstr(s,"artificial")) return TOPIC_AI;
    if (strstr(s,"cprog")||strstr(s,"c_prog")) return TOPIC_C_PROGRAMMING;
    if (strstr(s,"chemistry"))   return TOPIC_CHEMISTRY;
    if (strstr(s,"electronics")) return TOPIC_ELECTRONICS;
    return TOPIC_NONE;
}

/*
 * load_knowledge_base
 *
 * Parses edubot_data.txt into KB_STORE[].
 *
 * File format per entry:
 *   #TOPIC=<topic_name>
 *   #KEYS=<space separated keywords>
 *   <answer line 1>
 *   <answer line 2>
 *   ...
 *   ##END##
 *
 * Lines starting with # that are NOT #TOPIC or #KEYS are ignored.
 * Blank lines between ##END## and next #TOPIC are skipped.
 */
void load_knowledge_base(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr,
            "[WARNING] Dataset file '%s' not found.\n"
            "  Place edubot_data.txt in the same folder as edubot.exe\n",
            filename);
        return;
    }

    char         line[MAX_RESPONSE_LEN];
    KnowledgeItem cur;
    int           in_entry = 0;
    memset(&cur, 0, sizeof(cur));

    while (fgets(line, sizeof(line), fp)) {
        /* Strip trailing newline / carriage-return */
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1]=='\n' || line[len-1]=='\r'))
            line[--len] = '\0';

        if (strncmp(line, "#TOPIC=", 7) == 0) {
            /* Start a new entry */
            memset(&cur, 0, sizeof(cur));
            cur.topic = parse_topic_str(line + 7);
            in_entry  = 1;

        } else if (strncmp(line, "#UNIT=", 6) == 0) {
            cur.unit = atoi(line + 6);

        } else if (strncmp(line, "#KEYS=", 6) == 0) {
            strncpy(cur.keywords, line + 6, MAX_INPUT_LEN - 1);
            cur.keywords[MAX_INPUT_LEN - 1] = '\0';

        } else if (strcmp(line, "##END##") == 0) {
            /* Save completed entry */
            if (in_entry && cur.topic != TOPIC_NONE &&
                KB_COUNT < MAX_KB_ENTRIES) {
                KB_STORE[KB_COUNT++] = cur;
            }
            in_entry = 0;
            memset(&cur, 0, sizeof(cur));

        } else if (in_entry && line[0] != '\0') {
            /* Append line to answer */
            int cur_len = (int)strlen(cur.answer);
            if (cur_len > 0 && cur_len < MAX_RESPONSE_LEN - 2) {
                cur.answer[cur_len]   = '\n';
                cur.answer[cur_len+1] = '\0';
                cur_len++;
            }
            strncat(cur.answer, line, MAX_RESPONSE_LEN - cur_len - 1);
        }
    }

    fclose(fp);
    printf("  [EduBot] Loaded %d knowledge entries from '%s'\n\n",
           KB_COUNT, filename);
}

/*
 * find_best_match
 *
 * Scores every KB entry for the given topic against the processed
 * input using compute_similarity() on both keyword and answer fields,
 * and returns the highest-scoring answer above SIMILARITY_THRESHOLD.
 */
int find_best_match(const char *processed_input,
                    SubjectTopic topic,
                    int unit,
                    char *response_out)
{
    if (!processed_input || !response_out) return 0;

    float best      = 0.0f;
    int   best_idx  = -1;
    int   i;

    for (i = 0; i < KB_COUNT; i++) {
        if (KB_STORE[i].topic != topic) continue;
        /* unit filter: skip if entry has a unit set and it doesn't match */
        if (unit > 0 && KB_STORE[i].unit > 0 && KB_STORE[i].unit != unit) continue;

        float s1 = compute_similarity(processed_input, KB_STORE[i].keywords);
        /* Also score against first 256 chars of answer for richer matching */
        char ans_head[256];
        strncpy(ans_head, KB_STORE[i].answer, 255);
        ans_head[255] = '\0';
        float s2 = compute_similarity(processed_input, ans_head);
        float s  = s1 > s2 ? s1 : s2;
        /* boost exact unit match so unit-specific entries beat unit=0 generics */
        if (unit > 0 && KB_STORE[i].unit == unit) s *= 1.5f;
        if (s > 1.0f) s = 1.0f;

        if (s > best) { best = s; best_idx = i; }
    }

    if (best_idx >= 0 && best >= SIMILARITY_THRESHOLD) {
        strncpy(response_out, KB_STORE[best_idx].answer, MAX_RESPONSE_LEN - 1);
        response_out[MAX_RESPONSE_LEN - 1] = '\0';
        return 1;
    }

    strncpy(response_out,
        "I could not find a relevant answer for that query.\n"
        "  Tips:\n"
        "    - Use key words only (e.g. 'array definition' not 'what is an array')\n"
        "    - Try a different unit with 'units' command\n"
        "    - Type 'help' to see all commands",
        MAX_RESPONSE_LEN - 1);
    return 0;
}

/*
 * list_topics
 *
 * Builds a formatted list of every KB entry title that matches the
 * given topic+unit filter.  The "title" is the first line of the
 * answer text (everything before the first '\n').
 * Used by the help command to show students what they can ask about.
 */
void list_topics(SubjectTopic topic, int unit, char *response_out)
{
    if (!response_out) return;

    int pos = 0, count = 0, i;
    pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
        "\n  Topics you can ask about");

    if (unit > 0)
        pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
            " (Unit %d):\n", unit);
    else
        pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
            ":\n");

    pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
        "  ------------------------------------------------------------\n");

    for (i = 0; i < KB_COUNT && pos < MAX_RESPONSE_LEN - 80; i++) {
        if (KB_STORE[i].topic != topic) continue;
        if (unit > 0 && KB_STORE[i].unit > 0 && KB_STORE[i].unit != unit) continue;

        /* Extract first line of answer as the topic title */
        char title[128];
        const char *nl = strchr(KB_STORE[i].answer, '\n');
        int tlen = nl ? (int)(nl - KB_STORE[i].answer) : (int)strlen(KB_STORE[i].answer);
        if (tlen > 127) tlen = 127;
        strncpy(title, KB_STORE[i].answer, tlen);
        title[tlen] = '\0';
        /* Strip trailing colon if present */
        int tl = (int)strlen(title);
        if (tl > 0 && title[tl-1] == ':') title[--tl] = '\0';

        pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
            "  %2d. %s\n", ++count, title);
    }

    if (count == 0) {
        pos += snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
            "  (No topics loaded for this unit yet)\n");
    }

    snprintf(response_out + pos, MAX_RESPONSE_LEN - pos,
        "  ------------------------------------------------------------\n"
        "  Commands: quiz | units | menu | exit\n"
        "  Tip: type keywords from a topic title to ask about it.\n");
}

/* ================================================================== */
/*  QUIZ DATA — unit-tagged questions for each subject                */
/*  unit=0 means question applies to all units                        */
/* ================================================================== */

/* Helper macro to shorten struct init: topic, unit */
#define QQ(t,u) t,u

static const QuizQuestion PHYSICS_QUIZ[] = {
    /* Unit 1: Quantum Mechanics */
    {"de Broglie wavelength of a particle is:",
     {"A) h/mv","B) mv/h","C) h*mv","D) m/hv"},
     0,"lambda = h/p = h/(mv). Lighter/slower particle => larger wavelength.",QQ(TOPIC_PHYSICS,1)},
    {"Heisenberg uncertainty: delta_x * delta_p >=",
     {"A) h","B) h/2pi","C) h/(4pi)","D) 2h"},
     2,"Uncertainty principle: delta_x * delta_p >= h/(4*pi).",QQ(TOPIC_PHYSICS,1)},
    {"Energy of particle in a box: E_n is proportional to:",
     {"A) n","B) n^2","C) 1/n","D) sqrt(n)"},
     1,"E_n = n^2*h^2/(8ma^2). Energy scales as n^2.",QQ(TOPIC_PHYSICS,1)},
    {"At E = EF (Fermi level), probability of occupancy f(E) =",
     {"A) 0","B) 1","C) 0.5","D) depends on T"},
     2,"Fermi factor f(EF) = 1/[1+exp(0)] = 1/2 = 0.5 at ALL temperatures.",QQ(TOPIC_PHYSICS,1)},
    {"Photoelectric effect: the stopping potential depends on:",
     {"A) Intensity of light","B) Frequency of light","C) Distance from source","D) Angle of incidence"},
     1,"Stopping potential comes from photon energy hf minus work function: eVs = hf - phi. Independent of intensity.",QQ(TOPIC_PHYSICS,1)},
    {"According to de Broglie's hypothesis, matter waves are associated with:",
     {"A) Only charged particles","B) Only photons","C) All moving particles","D) Only electrons"},
     2,"de Broglie proposed that ALL moving particles (not just electrons) exhibit wave nature.",QQ(TOPIC_PHYSICS,1)},
    {"The Davisson-Germer experiment confirmed:",
     {"A) Photoelectric effect","B) Wave nature of electrons","C) Compton effect","D) Black body radiation"},
     1,"Electron diffraction by a nickel crystal confirmed de Broglie's matter-wave hypothesis.",QQ(TOPIC_PHYSICS,1)},
    {"Schrodinger's time-independent wave equation is used to find:",
     {"A) A particle's exact position","B) A particle's exact momentum","C) Allowed energy states/wavefunctions","D) A particle's charge"},
     2,"It is an eigenvalue equation whose solutions give the stationary energy states and wavefunctions.",QQ(TOPIC_PHYSICS,1)},
    {"The physical significance of |psi|^2 is:",
     {"A) Energy of the particle","B) Probability density of finding the particle","C) Momentum of the particle","D) Wavelength"},
     1,"Born's interpretation: |psi|^2 dV gives the probability of finding the particle within volume dV.",QQ(TOPIC_PHYSICS,1)},
    {"Zero-point energy of a particle in a 1D box arises because:",
     {"A) The particle is at rest","B) n cannot be zero (uncertainty principle)","C) The box has no walls","D) The particle has no mass"},
     1,"n=0 would make psi=0 everywhere (no particle), violating the uncertainty principle, so the minimum n=1 gives nonzero energy.",QQ(TOPIC_PHYSICS,1)},
    {"The Compton effect demonstrates that:",
     {"A) Light behaves purely as a wave","B) X-rays behave as particles (photons) colliding with electrons","C) Electrons behave purely as waves","D) Photons have no momentum"},
     1,"Compton scattering shows photons carry momentum p=h/lambda and collide with electrons like particles.",QQ(TOPIC_PHYSICS,1)},
    {"In Planck's radiation law, oscillator energy is quantized in units of:",
     {"A) mc^2","B) h*nu","C) kT","D) hc/lambda^2"},
     1,"Planck assumed oscillator energy is quantized as E = n*h*nu, n=0,1,2,...",QQ(TOPIC_PHYSICS,1)},
    {"Quantum mechanical tunneling allows a particle to:",
     {"A) Gain infinite energy","B) Cross a potential barrier higher than its own energy, with nonzero probability","C) Travel faster than light","D) Lose its wave nature"},
     1,"Tunneling gives a nonzero transmission probability through a classically forbidden barrier, due to the particle's wave nature.",QQ(TOPIC_PHYSICS,1)},
    {"For a particle in an infinite potential well, the wavefunction at the walls must be:",
     {"A) Infinite","B) Zero (boundary condition)","C) Maximum","D) Undefined"},
     1,"Since V=infinity outside the well, psi=0 at and beyond the walls.",QQ(TOPIC_PHYSICS,1)},
    {"The group velocity of a matter wave packet equals:",
     {"A) The phase velocity","B) The particle's classical velocity","C) The speed of light","D) Zero"},
     1,"The group velocity of the de Broglie wave packet equals the classical particle velocity v=p/m.",QQ(TOPIC_PHYSICS,1)},
    /* Unit 2: Electrical Conductivity */
    {"Classical free electron theory FAILS to explain:",
     {"A) Ohm's law","B) Specific heat of metals","C) Existence of current","D) Wire resistance"},
     1,"CFET predicts Cv=(3/2)R but experiment gives ~10^-4 RT. Quantum theory needed.",QQ(TOPIC_PHYSICS,2)},
    {"Hall coefficient RH is positive for:",
     {"A) N-type materials","B) P-type materials","C) Insulators","D) Superconductors"},
     1,"Positive RH means majority carriers are holes (p-type / some metals like Zn).",QQ(TOPIC_PHYSICS,2)},
    {"Forbidden energy gap of silicon is approximately:",
     {"A) 0.07 eV","B) 0.67 eV","C) 1.1 eV","D) 5.4 eV"},
     2,"Silicon Eg = 1.1 eV. Ge = 0.67 eV. Diamond (insulator) = 5.4 eV.",QQ(TOPIC_PHYSICS,2)},
    {"Drift velocity of electrons in a conductor is:",
     {"A) Their random thermal velocity","B) The average velocity gained due to an applied electric field","C) The speed of light in the medium","D) Always equal to the Fermi velocity"},
     1,"Drift velocity is the small net velocity superimposed on random thermal motion, due to an applied E-field.",QQ(TOPIC_PHYSICS,2)},
    {"The relaxation time (tau) in free electron theory represents:",
     {"A) Time between two successive collisions of an electron","B) Time for a wave to travel one wavelength","C) Lifetime of a photon","D) Time period of lattice vibration"},
     0,"Relaxation time is the average time between successive collisions of an electron with the lattice/impurities.",QQ(TOPIC_PHYSICS,2)},
    {"Mobility (mu) of electrons is defined as:",
     {"A) Drift velocity per unit electric field","B) Velocity per unit time","C) Charge per unit mass","D) Current per unit area"},
     0,"mu = v_d / E. Higher mobility means larger drift velocity for a given field.",QQ(TOPIC_PHYSICS,2)},
    {"According to the Wiedemann-Franz law, the ratio of thermal to electrical conductivity of a metal is:",
     {"A) Independent of temperature","B) Proportional to temperature","C) Inversely proportional to temperature","D) Proportional to temperature squared"},
     1,"K/sigma = L*T (Lorenz number L is constant), so the ratio is proportional to absolute temperature.",QQ(TOPIC_PHYSICS,2)},
    {"Matthiessen's rule states that the total resistivity of a metal is the sum of:",
     {"A) Ideal (phonon) and residual (impurity) resistivity","B) Electronic and ionic resistivity","C) Surface and bulk resistivity","D) Thermal and magnetic resistivity"},
     0,"rho_total = rho_ideal(T, due to phonons) + rho_residual (due to impurities/defects, T-independent).",QQ(TOPIC_PHYSICS,2)},
    {"As temperature increases, the resistivity of a pure metal:",
     {"A) Decreases","B) Increases (nearly linearly)","C) Remains constant","D) Becomes zero"},
     1,"More phonon scattering at higher T increases resistivity approximately linearly.",QQ(TOPIC_PHYSICS,2)},
    {"As temperature increases, the conductivity of an intrinsic semiconductor:",
     {"A) Decreases exponentially","B) Increases exponentially","C) Remains constant","D) Becomes zero"},
     1,"More electrons gain enough thermal energy to cross the gap, increasing carrier concentration exponentially with T.",QQ(TOPIC_PHYSICS,2)},
    {"The Hall effect is used to determine:",
     {"A) The type and concentration of charge carriers","B) Only the resistivity","C) Only the thermal conductivity","D) The speed of light in a material"},
     0,"The sign of the Hall voltage gives carrier type (n or p); its magnitude gives carrier concentration.",QQ(TOPIC_PHYSICS,2)},
    {"The Fermi velocity of electrons in a metal is the velocity of electrons:",
     {"A) At absolute zero, at the Fermi energy","B) At the speed of light","C) At rest","D) Only at very high temperature"},
     0,"Fermi velocity v_F corresponds to electrons occupying states right at the Fermi energy at T=0K.",QQ(TOPIC_PHYSICS,2)},
    {"Ohm's law in microscopic (vector) form is written as:",
     {"A) V = IR","B) J = sigma * E","C) P = I^2R","D) E = mc^2"},
     1,"Current density J is proportional to the electric field E, with conductivity sigma as the constant: J = sigma*E.",QQ(TOPIC_PHYSICS,2)},
    {"The mean free path of an electron is the average distance travelled:",
     {"A) In one second","B) Between two successive collisions","C) Across the entire conductor","D) During one oscillation"},
     1,"Mean free path (lambda) = v * tau, the average distance an electron travels between collisions.",QQ(TOPIC_PHYSICS,2)},
    {"Classical free electron theory (CFET) treats free electrons as behaving like:",
     {"A) Quantum particles obeying Pauli exclusion","B) A classical ideal gas (Maxwell-Boltzmann statistics)","C) Waves only","D) Photons"},
     1,"CFET treats electrons as a classical ideal gas, unlike quantum free electron theory which uses Fermi-Dirac statistics.",QQ(TOPIC_PHYSICS,2)},
    /* Unit 3: Superconductivity */
    {"The Meissner effect refers to:",
     {"A) Zero resistance","B) Expulsion of magnetic flux","C) Phonon-mediated pairing","D) Energy gap at Fermi level"},
     1,"Meissner effect: superconductor expels all magnetic flux (B=0 inside).",QQ(TOPIC_PHYSICS,3)},
    {"Cooper pairs in BCS theory are formed by:",
     {"A) Direct Coulomb attraction","B) Phonon-mediated interaction","C) Magnetic force","D) Gravity"},
     1,"BCS: electron-phonon-electron interaction creates an effective attractive force.",QQ(TOPIC_PHYSICS,3)},
    {"Type II superconductors differ from Type I because they have:",
     {"A) Higher resistance","B) No Meissner effect","C) Two critical fields Hc1 and Hc2","D) Only one critical temperature"},
     2,"Type II: partial flux penetration (vortex state) between Hc1 and Hc2.",QQ(TOPIC_PHYSICS,3)},
    {"The temperature below which a material becomes superconducting is called:",
     {"A) Debye temperature","B) Curie temperature","C) Critical temperature (Tc)","D) Boiling point"},
     2,"Tc is the characteristic transition temperature below which resistance drops to exactly zero.",QQ(TOPIC_PHYSICS,3)},
    {"The isotope effect in superconductivity shows that Tc is proportional to:",
     {"A) M (isotopic mass)","B) 1/sqrt(M)","C) M^2","D) Independent of mass"},
     1,"Tc ~ M^(-1/2), supporting phonon involvement (BCS theory) since heavier isotopes vibrate more slowly.",QQ(TOPIC_PHYSICS,3)},
    {"The maximum current a superconductor can carry without losing superconductivity is called:",
     {"A) Critical current","B) Saturation current","C) Threshold current","D) Drift current"},
     0,"Beyond the critical current density Jc, the material reverts to its normal (resistive) state.",QQ(TOPIC_PHYSICS,3)},
    {"London penetration depth refers to:",
     {"A) The depth to which a magnetic field penetrates a superconductor","B) The thickness of the superconducting wire","C) The distance between Cooper pairs","D) The depth of the Fermi sea"},
     0,"A magnetic field decays exponentially inside a superconductor over the London penetration depth lambda_L.",QQ(TOPIC_PHYSICS,3)},
    {"High-Tc superconductors like YBCO are typically:",
     {"A) Pure metals","B) Ceramic cuprate compounds","C) Elemental semiconductors","D) Alloys of iron only"},
     1,"YBa2Cu3O7 (YBCO) is a ceramic cuprate superconductor with Tc above liquid-nitrogen temperature (77K).",QQ(TOPIC_PHYSICS,3)},
    {"The Josephson effect refers to:",
     {"A) Tunneling of Cooper pairs through a thin insulating barrier between two superconductors","B) Expulsion of magnetic flux","C) Zero resistance at high temperature","D) Loss of superconductivity under pressure"},
     0,"Cooper pairs tunnel through a thin insulating junction, producing a supercurrent with no applied voltage.",QQ(TOPIC_PHYSICS,3)},
    {"The energy gap in BCS theory represents:",
     {"A) The energy needed to break a Cooper pair","B) The Fermi energy","C) The work function","D) The band gap of a semiconductor"},
     0,"The BCS energy gap 2*Delta is the minimum energy required to break a bound Cooper pair into two normal electrons.",QQ(TOPIC_PHYSICS,3)},
    {"A superconductor in the Meissner state behaves as a:",
     {"A) Paramagnet","B) Perfect diamagnet","C) Ferromagnet","D) Non-magnetic insulator"},
     1,"Complete flux expulsion (B=0 inside) means the superconductor exhibits perfect diamagnetism.",QQ(TOPIC_PHYSICS,3)},
    {"Superconducting magnets used in MRI machines mainly exploit which property?",
     {"A) Zero electrical resistance for very high sustained current","B) High optical transparency","C) High Hall coefficient","D) High thermal conductivity"},
     0,"Zero resistance allows persistent, lossless high currents needed to generate strong magnetic fields.",QQ(TOPIC_PHYSICS,3)},
    {"Coherence length in superconductivity refers to:",
     {"A) The wavelength of emitted photons","B) The spatial extent (size) of a Cooper pair","C) The length of the superconducting wire","D) The mean free path of phonons"},
     1,"Coherence length (xi) is the characteristic distance over which the Cooper pair correlation extends.",QQ(TOPIC_PHYSICS,3)},
    {"Which of these is an application exploiting the Meissner effect?",
     {"A) Maglev (magnetic levitation) trains","B) LED lighting","C) Solar cells","D) Optical fibers"},
     0,"Flux expulsion causes a strong repulsive force enabling frictionless magnetic levitation.",QQ(TOPIC_PHYSICS,3)},
    {"Type I superconductors, compared to Type II, have:",
     {"A) Two critical magnetic fields","B) A single, relatively low critical magnetic field","C) No critical field","D) A higher critical temperature always"},
     1,"Type I superconductors have one critical field Hc and lose superconductivity abruptly above it.",QQ(TOPIC_PHYSICS,3)},
    /* Unit 4: Lasers & Optical Fibers */
    {"Laser action requires:",
     {"A) Population inversion","B) Thermal equilibrium","C) Ground state dominance","D) High pressure"},
     0,"Population inversion (N2>N1) is essential for stimulated emission to dominate.",QQ(TOPIC_PHYSICS,4)},
    {"Ruby laser output wavelength is approximately:",
     {"A) 532 nm (green)","B) 694 nm (red)","C) 1064 nm (infrared)","D) 488 nm (blue)"},
     1,"Ruby laser emits at 694.3 nm (deep red). Pulsed output, three-level system.",QQ(TOPIC_PHYSICS,4)},
    {"Numerical Aperture (NA) of an optical fiber equals:",
     {"A) n1/n2","B) n1-n2","C) sqrt(n1^2 - n2^2)","D) n1*n2"},
     2,"NA = sqrt(n1^2 - n2^2). Determines acceptance angle of the fiber.",QQ(TOPIC_PHYSICS,4)},
    {"Stimulated emission differs from spontaneous emission because it produces photons that are:",
     {"A) Random in phase and direction","B) Coherent — same phase, direction, and frequency as the incident photon","C) Always of higher energy","D) Always polarised differently"},
     1,"Stimulated emission produces a photon identical to the one that triggered it, giving coherent laser light.",QQ(TOPIC_PHYSICS,4)},
    {"Einstein's A and B coefficients describe:",
     {"A) Absorption, spontaneous emission, and stimulated emission rates","B) Only spontaneous emission","C) Only fiber attenuation","D) Refractive index"},
     0,"Coefficients A (spontaneous emission) and B (absorption/stimulated emission) describe these radiative processes.",QQ(TOPIC_PHYSICS,4)},
    {"A resonant (optical) cavity in a laser is used to:",
     {"A) Absorb excess photons","B) Provide optical feedback so light is repeatedly amplified","C) Cool the laser medium","D) Convert light to electrical energy"},
     1,"Mirrors at each end reflect photons back through the gain medium repeatedly, amplifying the beam.",QQ(TOPIC_PHYSICS,4)},
    {"The He-Ne laser typically emits visible red light at:",
     {"A) 488 nm","B) 532 nm","C) 632.8 nm","D) 780 nm"},
     2,"The common He-Ne laser transition emits at 632.8 nm (red).",QQ(TOPIC_PHYSICS,4)},
    {"Laser light being highly monochromatic means:",
     {"A) It has a single, well-defined polarisation","B) It has an extremely narrow range of wavelengths","C) It travels only in straight lines","D) It has zero intensity"},
     1,"Monochromaticity means the emitted light is confined to a very narrow spectral linewidth.",QQ(TOPIC_PHYSICS,4)},
    {"In a semiconductor (diode) laser, population inversion is achieved by:",
     {"A) Optical pumping with a flash lamp","B) Forward-biasing a p-n junction to inject carriers","C) A chemical reaction","D) Passing current through a gas discharge tube"},
     1,"Forward bias injects electrons and holes into the active region, creating population inversion at the junction.",QQ(TOPIC_PHYSICS,4)},
    {"In a step-index optical fiber, the core's refractive index is:",
     {"A) Lower than the cladding","B) Equal to the cladding","C) Higher than the cladding, and constant across the core","D) Varying (parabolic) across the core"},
     2,"Step-index fiber has a uniform, higher-index core surrounded by lower-index cladding for total internal reflection.",QQ(TOPIC_PHYSICS,4)},
    {"A graded-index optical fiber differs from step-index because its core refractive index:",
     {"A) Decreases gradually from the axis to the cladding","B) Is constant","C) Is lower than the cladding","D) Has no cladding"},
     0,"In graded-index fiber, refractive index decreases gradually (parabolically) outward, reducing modal dispersion.",QQ(TOPIC_PHYSICS,4)},
    {"The acceptance angle of an optical fiber is:",
     {"A) The angle at which the fiber breaks","B) The maximum incidence angle for which light is guided by total internal reflection","C) The angle between core and cladding","D) Always 90 degrees"},
     1,"Light entering within the acceptance cone is guided by total internal reflection; sin(acceptance angle)=NA.",QQ(TOPIC_PHYSICS,4)},
    {"Attenuation in optical fibers is mainly caused by:",
     {"A) Population inversion","B) Absorption and scattering (e.g., Rayleigh scattering) losses","C) The laser's coherence","D) The cladding's magnetic properties"},
     1,"Signal loss along the fiber arises chiefly from material absorption and Rayleigh scattering.",QQ(TOPIC_PHYSICS,4)},
    {"The V-number (normalized frequency) of an optical fiber determines:",
     {"A) The number of modes the fiber can support","B) The color of light used","C) The type of laser used","D) The core's electrical conductivity"},
     0,"V-number determines whether a fiber is single-mode (V<2.405) or multimode, and roughly how many modes propagate.",QQ(TOPIC_PHYSICS,4)},
    {"A four-level laser system is generally more efficient than a three-level system because:",
     {"A) It requires less pump power to achieve population inversion","B) It uses no mirrors","C) It emits only infrared light","D) It has no metastable state"},
     0,"The lower laser level is normally empty in a four-level system, so inversion needs far less pump energy than in a three-level system like ruby.",QQ(TOPIC_PHYSICS,4)},
    /* Unit 5: Semiconductor Physics */
    {"An intrinsic semiconductor is one that is:",
     {"A) Heavily doped with impurities","B) Pure, with equal electron and hole concentrations","C) A perfect conductor","D) Always at absolute zero"},
     1,"Intrinsic (pure) semiconductors have equal numbers of thermally generated electrons and holes (n=p=ni).",QQ(TOPIC_PHYSICS,5)},
    {"Adding pentavalent (Group V) impurities like Phosphorus to silicon creates a:",
     {"A) P-type semiconductor","B) N-type semiconductor","C) Intrinsic semiconductor","D) Insulator"},
     1,"Pentavalent dopants donate a free extra electron, creating an n-type (electron-majority) semiconductor.",QQ(TOPIC_PHYSICS,5)},
    {"Adding trivalent (Group III) impurities like Boron to silicon creates a:",
     {"A) N-type semiconductor","B) P-type semiconductor","C) Metal","D) Intrinsic semiconductor"},
     1,"Trivalent dopants create a hole (electron deficiency), giving a p-type (hole-majority) semiconductor.",QQ(TOPIC_PHYSICS,5)},
    {"In an intrinsic semiconductor, the Fermi level lies:",
     {"A) At the conduction band edge","B) At the valence band edge","C) Approximately at the middle of the forbidden gap","D) Outside the band gap"},
     2,"For an intrinsic semiconductor with comparable effective masses, EF sits near the midgap between Ec and Ev.",QQ(TOPIC_PHYSICS,5)},
    {"In an n-type semiconductor, the Fermi level shifts:",
     {"A) Towards the valence band","B) Towards the conduction band","C) To the exact middle of the gap","D) Below the valence band"},
     1,"Extra donor electrons raise the Fermi level closer to the conduction band edge.",QQ(TOPIC_PHYSICS,5)},
    {"In a p-type semiconductor, the Fermi level shifts:",
     {"A) Towards the conduction band","B) Towards the valence band","C) Out of the crystal","D) To infinity"},
     1,"Excess holes (acceptor levels) lower the Fermi level closer to the valence band edge.",QQ(TOPIC_PHYSICS,5)},
    {"A direct bandgap semiconductor (e.g. GaAs) is preferred for LEDs and lasers because:",
     {"A) Electron-hole recombination directly emits a photon without needing phonon assistance","B) It has no bandgap","C) It conducts like a metal","D) It cannot absorb light"},
     0,"In direct-gap materials, momentum is conserved without phonon involvement, making radiative recombination efficient.",QQ(TOPIC_PHYSICS,5)},
    {"Silicon and Germanium are examples of:",
     {"A) Direct bandgap semiconductors","B) Indirect bandgap semiconductors","C) Insulators","D) Superconductors"},
     1,"Si and Ge have indirect band gaps — recombination needs phonon assistance, making them inefficient light emitters.",QQ(TOPIC_PHYSICS,5)},
    {"The law of mass action for a semiconductor states that:",
     {"A) n + p = constant","B) n * p = ni^2 (constant at a given temperature)","C) n - p = 0 always","D) n/p = 1 always"},
     1,"Regardless of doping, the product of electron and hole concentrations equals ni^2 at thermal equilibrium.",QQ(TOPIC_PHYSICS,5)},
    {"In silicon, compared to holes, electrons have:",
     {"A) Lower mobility","B) Higher mobility","C) Equal mobility","D) No mobility"},
     1,"Electron mobility in silicon (~1350 cm^2/V-s) is significantly higher than hole mobility (~480 cm^2/V-s).",QQ(TOPIC_PHYSICS,5)},
    {"As temperature increases, the resistivity of an intrinsic semiconductor:",
     {"A) Increases","B) Decreases","C) Stays constant","D) Becomes infinite"},
     1,"More electron-hole pairs are thermally generated at higher T, increasing conductivity — opposite of metals.",QQ(TOPIC_PHYSICS,5)},
    {"The depletion region in a p-n junction is:",
     {"A) A region rich in free charge carriers","B) A region depleted of free carriers, containing fixed ions","C) The metal contact area","D) Outside the semiconductor"},
     1,"Diffusion of carriers across the junction leaves behind fixed ionized dopant atoms, depleting free carriers.",QQ(TOPIC_PHYSICS,5)},
    {"In a semiconductor, diffusion current arises due to:",
     {"A) An applied electric field only","B) A concentration gradient of carriers","C) A magnetic field","D) Temperature being zero"},
     1,"Diffusion current flows from high to low carrier concentration, independent of any applied field.",QQ(TOPIC_PHYSICS,5)},
    {"Drift current in a semiconductor is caused by:",
     {"A) A concentration gradient","B) An applied electric field acting on carriers","C) Recombination only","D) Photon absorption only"},
     1,"Drift current results from carriers being accelerated by an applied electric field: J_drift = q*n*mu*E.",QQ(TOPIC_PHYSICS,5)},
    {"A semiconductor is called 'degenerate' when:",
     {"A) It has no free carriers","B) Heavy doping pushes the Fermi level into the conduction or valence band","C) It is undoped","D) It has zero bandgap"},
     1,"Very heavy doping pushes EF into the band itself, causing the material to behave more like a metal.",QQ(TOPIC_PHYSICS,5)}
};

static const QuizQuestion CIVIL_QUIZ[] = {
    {"How many main types of staircases are there?",
     {"A) 4","B) 6","C) 8","D) 10"},
     2,"8 types: straight, dog-legged, open-well, spiral, bifurcated, quarter-turn, three-quarter turn, circular.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Minimum concrete grade for RCC per IS 456:",
     {"A) M10","B) M15","C) M20","D) M25"},
     2,"IS 456 mandates M20 minimum for RCC.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Dog-legged staircase has how many flights?",
     {"A) 1","B) 2","C) 3","D) 4"},
     1,"Dog-legged staircase has two flights turning 180 degrees at a common landing.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Slump cone height per IS 1199:",
     {"A) 200mm","B) 250mm","C) 300mm","D) 350mm"},
     2,"Slump cone: bottom=200mm, top=100mm, height=300mm.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Which dam type resists water pressure by its own weight?",
     {"A) Arch dam","B) Gravity dam","C) Earthen dam","D) Buttress dam"},
     1,"Gravity dam relies on its own massive weight to resist water pressure.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Ordinary Portland Cement's final setting time, per IS specification, should not exceed:",
     {"A) 30 minutes","B) 10 hours (600 minutes)","C) 24 hours","D) 5 minutes"},
     1,"IS 4031/IS 269 specify final setting time of OPC should not exceed 600 minutes (10 hours).",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"The water-cement ratio in a concrete mix primarily affects:",
     {"A) Only the color of concrete","B) The strength and workability of concrete","C) Only the curing time","D) The cost of aggregates"},
     1,"A lower w/c ratio generally gives higher strength but reduced workability — a critical mix-design parameter.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Curing of concrete is done to:",
     {"A) Speed up setting","B) Maintain adequate moisture/temperature for proper hydration and strength gain","C) Reduce cement content","D) Add color"},
     1,"Curing keeps concrete moist so cement hydration continues, developing full design strength.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"A Damp Proof Course (DPC) is provided in a building to:",
     {"A) Prevent rising dampness/moisture from the ground into walls","B) Increase strength of the foundation","C) Reduce the cost of construction","D) Improve the aesthetic appearance"},
     0,"DPC is a waterproof layer laid at plinth level to block capillary rise of ground moisture into the walls.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Which foundation type spreads load over the entire building footprint, suiting weak soils and heavy loads?",
     {"A) Isolated footing","B) Combined footing","C) Raft (mat) foundation","D) Strip footing"},
     2,"A raft foundation spreads the load over the entire building base, suitable for weak soils and heavy loads.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Pile foundations are typically used when:",
     {"A) Surface soil has adequate bearing capacity","B) Hard/strong strata is available only at greater depth","C) The building is a single-storey structure","D) Cost must be minimized"},
     1,"Piles transfer loads down to a strong bearing stratum (or via skin friction) when shallow soil is too weak.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"In brickwork, 'English bond' is preferred over 'Stretcher bond' for load-bearing walls because it:",
     {"A) Uses fewer bricks","B) Provides greater strength via alternating header and stretcher courses","C) Is only decorative","D) Requires no mortar"},
     1,"English bond alternates full header and stretcher courses, giving strong, well-interlocked walls.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"Chain surveying is most suitable for:",
     {"A) Large hilly areas with obstacles","B) Small, fairly flat areas with few obstructions","C) Underwater surveying","D) Astronomical surveying"},
     1,"Chain surveying uses only linear measurements (chain/tape), so it works best on small, open, flat terrain.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"A Total Station instrument in surveying combines the functions of:",
     {"A) A compass and a hammer","B) An electronic theodolite and an electronic distance measuring (EDM) device","C) A level and a ruler only","D) A drone and a camera"},
     1,"A Total Station integrates angle measurement (theodolite) with electronic distance measurement in one instrument.",QQ(TOPIC_CIVIL_ENGINEERING,0)},
    {"BOD (Biochemical Oxygen Demand) in water/wastewater testing measures:",
     {"A) The dissolved oxygen consumed by microorganisms decomposing organic matter","B) The total volume of water","C) The pH level","D) The temperature of water"},
     0,"BOD indicates the oxygen microbes need to break down organic pollutants — a key water-quality indicator.",QQ(TOPIC_CIVIL_ENGINEERING,0)}
};

static const QuizQuestion MATHS_QUIZ[] = {
    /* Unit 1: Sequences & Series */
    {"A sequence is said to converge if:",
     {"A) It oscillates forever","B) Its terms approach a finite limit as n->infinity","C) It has infinite terms","D) It is always increasing"},
     1,"Convergence means lim(n->infinity) a_n = L for some finite L.",QQ(TOPIC_MATHS,1)},
    {"The sum of an infinite geometric series with |r|<1 and first term a is:",
     {"A) a/(1-r)","B) a*r","C) a/(1+r)","D) a*n"},
     0,"S = a/(1-r) for |r|<1; the series diverges otherwise.",QQ(TOPIC_MATHS,1)},
    {"The harmonic series (sum of 1/n) is:",
     {"A) Convergent","B) Divergent","C) Equal to zero","D) Equal to 1"},
     1,"Despite terms tending to 0, the harmonic series diverges (grows like ln n).",QQ(TOPIC_MATHS,1)},
    {"A p-series (sum of 1/n^p) converges if and only if:",
     {"A) p<1","B) p=1","C) p>1","D) p is negative"},
     2,"p-series converges for p>1, diverges for p<=1.",QQ(TOPIC_MATHS,1)},
    {"The ratio test examines L = lim|a_(n+1)/a_n|. The series converges if:",
     {"A) L>1","B) L<1","C) L=1","D) L=infinity"},
     1,"If L<1 the series converges absolutely; L>1 diverges; L=1 is inconclusive.",QQ(TOPIC_MATHS,1)},
    {"An alternating series converges (by the Leibniz test) if its terms:",
     {"A) Increase in magnitude","B) Decrease monotonically to zero","C) Are always positive","D) Are constant"},
     1,"Leibniz's test: an alternating series converges if |a_n| decreases monotonically to 0.",QQ(TOPIC_MATHS,1)},
    {"A sequence that is bounded and monotonic is guaranteed to be:",
     {"A) Divergent","B) Convergent","C) Zero","D) Undefined"},
     1,"The Monotone Convergence Theorem: bounded + monotonic implies convergent.",QQ(TOPIC_MATHS,1)},
    {"The Maclaurin series is a special case of the Taylor series expanded about:",
     {"A) x=1","B) x=infinity","C) x=0","D) x=a (arbitrary point)"},
     2,"The Maclaurin series is the Taylor series centered at x=0.",QQ(TOPIC_MATHS,1)},
    {"The radius of convergence of a power series is typically found using:",
     {"A) The mean value theorem","B) The ratio test (or root test)","C) L'Hopital's rule","D) Integration by parts"},
     1,"Applying the ratio/root test to the power series terms gives the radius of convergence R.",QQ(TOPIC_MATHS,1)},
    {"For the comparison test, if 0<=a_n<=b_n and sum(b_n) converges, then sum(a_n):",
     {"A) Diverges","B) Also converges","C) Cannot be determined","D) Must equal sum(b_n)"},
     1,"A nonnegative series bounded above (term-by-term) by a convergent series must also converge.",QQ(TOPIC_MATHS,1)},
    {"The Maclaurin series expansion of e^x is:",
     {"A) 1+x+x^2/2!+x^3/3!+...","B) x-x^3/3!+x^5/5!-...","C) 1-x+x^2-x^3+...","D) x+x^2+x^3+..."},
     0,"e^x = sum(x^n/n!) for n=0 to infinity.",QQ(TOPIC_MATHS,1)},
    {"A sequence {a_n} is called a Cauchy sequence if:",
     {"A) It is always decreasing","B) Terms get arbitrarily close to each other as n,m->infinity","C) It has finitely many terms","D) It never converges"},
     1,"Cauchy criterion: for any epsilon>0 there is N such that |a_n-a_m|<epsilon for n,m>N; equivalent to convergence in R.",QQ(TOPIC_MATHS,1)},
    {"The Maclaurin series for sin(x) is:",
     {"A) 1-x^2/2!+x^4/4!-...","B) x-x^3/3!+x^5/5!-...","C) x+x^3/3!+x^5/5!+...","D) 1+x+x^2+..."},
     1,"sin(x) = x - x^3/3! + x^5/5! - ... (odd powers, alternating signs).",QQ(TOPIC_MATHS,1)},
    {"An arithmetic series has a common:",
     {"A) Ratio between terms","B) Difference between terms","C) Product of terms","D) Logarithm of terms"},
     1,"Arithmetic sequences/series have a constant difference d between consecutive terms.",QQ(TOPIC_MATHS,1)},
    {"If lim(n->infinity) a_n != 0 for a series sum(a_n), then the series:",
     {"A) Converges","B) Diverges (nth term test)","C) Is geometric","D) Is a p-series"},
     1,"The nth-term (divergence) test: if terms don't tend to zero, the series must diverge.",QQ(TOPIC_MATHS,1)},
    /* Unit 2: Vector Differentiation */
    {"If |A| is constant, then A . (dA/dt) =",
     {"A) |A|","B) 1","C) 0","D) |A|^2"},
     2,"If magnitude is constant, A is perpendicular to its derivative, so dot product = 0.",QQ(TOPIC_MATHS,2)},
    {"div(F) = 0 means F is:",
     {"A) Irrotational","B) Solenoidal","C) Conservative","D) Scalar"},
     1,"div(F)=0 means solenoidal (no sources/sinks). curl(F)=0 means irrotational.",QQ(TOPIC_MATHS,2)},
    {"curl(grad(phi)) is always:",
     {"A) grad(phi)","B) div(phi)","C) zero vector","D) phi"},
     2,"curl of any gradient is identically zero. This is a vector identity.",QQ(TOPIC_MATHS,2)},
    {"The gradient of a scalar field phi points in the direction of:",
     {"A) Zero rate of change","B) Maximum rate of increase of phi","C) Minimum rate of increase","D) The z-axis always"},
     1,"grad(phi) points in the direction of steepest ascent (maximum increase) of the scalar field.",QQ(TOPIC_MATHS,2)},
    {"The directional derivative of phi along unit vector n-hat is given by:",
     {"A) grad(phi) . n-hat","B) grad(phi) x n-hat","C) div(phi)","D) curl(phi)"},
     0,"The directional derivative equals grad(phi) dotted with the unit direction vector.",QQ(TOPIC_MATHS,2)},
    {"The del operator (nabla) applied to a scalar field gives the:",
     {"A) Divergence","B) Curl","C) Gradient","D) Laplacian"},
     2,"nabla*phi (applied to a scalar) produces the gradient vector field.",QQ(TOPIC_MATHS,2)},
    {"Physically, curl(F) at a point represents:",
     {"A) The flux of F through a surface","B) The rotation/circulation tendency of F at that point","C) The magnitude of F","D) The potential energy"},
     1,"curl(F) measures the local rotation (angular velocity) of the vector field.",QQ(TOPIC_MATHS,2)},
    {"Physically, div(F) at a point represents:",
     {"A) The rotation of F","B) The net outward flux (source/sink strength) per unit volume","C) The gradient of F","D) The magnitude of F"},
     1,"Divergence measures how much a vector field spreads out from (source) or converges into (sink) a point.",QQ(TOPIC_MATHS,2)},
    {"A vector field F is irrotational if:",
     {"A) div(F)=0","B) curl(F)=0","C) grad(F)=0","D) F=0 everywhere"},
     1,"Irrotational means curl(F)=vec 0 everywhere; such fields can be written as F=grad(phi).",QQ(TOPIC_MATHS,2)},
    {"The gradient vector grad(phi) at a point is always:",
     {"A) Tangent to the level surface phi=constant","B) Normal (perpendicular) to the level surface","C) Zero on level surfaces","D) Parallel to the x-axis"},
     1,"grad(phi) is perpendicular to the level surface passing through that point.",QQ(TOPIC_MATHS,2)},
    {"The Laplacian operator del^2(phi) is defined as:",
     {"A) div(grad(phi))","B) curl(grad(phi))","C) grad(div(phi))","D) grad(curl(phi))"},
     0,"Laplacian = div(grad(phi)) = d2phi/dx2 + d2phi/dy2 + d2phi/dz2.",QQ(TOPIC_MATHS,2)},
    {"The identity div(curl(F)) for any vector field F is always:",
     {"A) Equal to F","B) Zero","C) Equal to grad(F)","D) Undefined"},
     1,"div(curl(F))=0 is a fundamental vector identity — curl fields have no sources/sinks.",QQ(TOPIC_MATHS,2)},
    {"A vector field F is called conservative if it can be expressed as:",
     {"A) F = curl(G) for some G","B) F = grad(phi) for some scalar phi","C) F = div(phi)","D) F is always zero"},
     1,"Conservative fields are gradients of a scalar potential function phi.",QQ(TOPIC_MATHS,2)},
    {"For a scalar field phi = x^2+y^2+z^2, grad(phi) equals:",
     {"A) 2x+2y+2z","B) 2xi+2yj+2zk","C) x^2i+y^2j+z^2k","D) 6"},
     1,"grad(phi) = (dphi/dx)i+(dphi/dy)j+(dphi/dz)k = 2xi+2yj+2zk.",QQ(TOPIC_MATHS,2)},
    {"The unit normal vector to a surface phi(x,y,z)=c at a point is given by:",
     {"A) grad(phi)","B) grad(phi) / |grad(phi)|","C) curl(phi)","D) div(phi)"},
     1,"The unit normal is the gradient vector normalized by its own magnitude: n-hat = grad(phi)/|grad(phi)|.",QQ(TOPIC_MATHS,2)},
    /* Unit 3: Vector Integration */
    {"Green's theorem converts a line integral to a:",
     {"A) Volume integral","B) Surface integral","C) Double integral","D) Triple integral"},
     2,"Green's theorem: closed line integral = double integral over the enclosed region.",QQ(TOPIC_MATHS,3)},
    {"Gauss's divergence theorem relates surface integral to:",
     {"A) Line integral","B) Volume integral","C) Double integral","D) Path integral"},
     1,"Gauss: flux through closed surface = volume integral of divergence inside.",QQ(TOPIC_MATHS,3)},
    {"Stokes' theorem: oint_C F.dr = iint_S (curl F).dS. C is the ___ of S.",
     {"A) Interior","B) Normal","C) Boundary","D) Gradient"},
     2,"Stokes: C is the boundary curve of surface S.",QQ(TOPIC_MATHS,3)},
    {"A line integral of a vector field F along curve C, integral(F.dr), physically represents:",
     {"A) The area under the curve","B) The work done by the force field F along C","C) The volume enclosed","D) The mass of the curve"},
     1,"For a force field, the line integral of F.dr gives the work done moving along C.",QQ(TOPIC_MATHS,3)},
    {"If a vector field F is conservative, the line integral between two points is:",
     {"A) Path-dependent","B) Path-independent, depending only on the endpoints","C) Always zero","D) Undefined"},
     1,"For conservative fields, integral(F.dr) = phi(B)-phi(A), independent of the path taken.",QQ(TOPIC_MATHS,3)},
    {"The line integral of a conservative field around any closed path is:",
     {"A) Infinite","B) Zero","C) Equal to the enclosed area","D) Negative"},
     1,"The closed-loop integral of a conservative (irrotational) field is always zero.",QQ(TOPIC_MATHS,3)},
    {"Green's theorem relates a line integral around closed curve C to a double integral over the region:",
     {"A) Outside C","B) Enclosed by C","C) Along the z-axis","D) At infinity"},
     1,"Green's theorem: oint(Pdx+Qdy) = double_integral(dQ/dx - dP/dy) dA over the enclosed region.",QQ(TOPIC_MATHS,3)},
    {"Gauss's divergence theorem states that the flux of F through a closed surface S equals:",
     {"A) The line integral around the boundary of S","B) The volume integral of div(F) over the enclosed volume","C) The surface area of S","D) Zero always"},
     1,"Gauss: closed-surface flux integral(F.n dS) = volume_integral(div F) dV.",QQ(TOPIC_MATHS,3)},
    {"Stokes' theorem converts a line integral around closed curve C into a:",
     {"A) Volume integral","B) Surface integral of curl(F) over any surface bounded by C","C) Another line integral","D) A scalar constant"},
     1,"Stokes: oint_C F.dr = double_integral_S (curl F).dS, for any surface S with boundary C.",QQ(TOPIC_MATHS,3)},
    {"A surface integral of the form integral(F.n dS) represents the:",
     {"A) Circulation of F","B) Flux of F through the surface","C) Work done along a curve","D) Length of the surface"},
     1,"This is the flux integral, measuring the flow of the vector field through the surface.",QQ(TOPIC_MATHS,3)},
    {"In Stokes' theorem, the orientation of boundary curve C must be consistent with:",
     {"A) The clock direction only","B) The right-hand rule relative to the surface normal","C) Gravity","D) The x-axis direction"},
     1,"The right-hand rule relates the traversal direction of C to the chosen normal direction of S.",QQ(TOPIC_MATHS,3)},
    {"Circulation of a vector field around a closed curve C is defined as:",
     {"A) integral(F.n dS)","B) oint_C F.dr","C) div(F)","D) grad(F)"},
     1,"Circulation = the closed line integral oint_C F.dr, measuring net rotational flow.",QQ(TOPIC_MATHS,3)},
    {"The divergence theorem is a 3D generalisation of:",
     {"A) The fundamental theorem of calculus","B) L'Hopital's rule","C) The chain rule","D) Taylor series"},
     0,"Gauss's theorem generalises the fundamental theorem of calculus (integral of a derivative = boundary values) to 3D flux/divergence.",QQ(TOPIC_MATHS,3)},
    {"For F=xi+yj+zk, computing the flux through a closed sphere via the divergence theorem requires:",
     {"A) curl(F)","B) div(F) integrated over the sphere's volume","C) grad(F)","D) The surface area only"},
     1,"div(F)=3 here (constant), so flux = 3 * (volume of the sphere) by Gauss's theorem.",QQ(TOPIC_MATHS,3)},
    {"A scalar line integral integral(f ds) differs from a vector line integral because it:",
     {"A) Uses arc length ds instead of dr, giving a scalar result","B) Always gives zero","C) Requires a closed curve","D) Cannot be computed"},
     0,"Scalar line integrals use ds (arc length element) and produce a scalar, without a dot product with direction.",QQ(TOPIC_MATHS,3)},
    /* Unit 4: Linear ODEs */
    {"For auxiliary equation roots m = 2+3i, 2-3i, the complementary function is:",
     {"A) e^2x(C1cos3x+C2sin3x)","B) (C1+C2x)e^2x","C) C1e^2x+C2e^3x","D) e^3x(C1cos2x+C2sin2x)"},
     0,"Complex roots a+/-bi -> yc = e^(ax)[C1*cos(bx)+C2*sin(bx)].",QQ(TOPIC_MATHS,4)},
    {"1/F(D) * e^(ax) when F(a)=0 equals:",
     {"A) e^(ax)/F(a)","B) x*e^(ax)/F'(a)","C) e^(ax)*F(a)","D) 0"},
     1,"If F(a)=0, multiply by x: yp = x*e^(ax)/F'(a).",QQ(TOPIC_MATHS,4)},
    {"The order of a differential equation is defined as:",
     {"A) The degree of the highest derivative","B) The order of the highest derivative present","C) The number of terms in the equation","D) The coefficient of y"},
     1,"Order = order of the highest-order derivative appearing, e.g. y''+y=0 has order 2.",QQ(TOPIC_MATHS,4)},
    {"A linear ODE with constant coefficients where the RHS = 0 is called:",
     {"A) Non-homogeneous","B) Homogeneous","C) Non-linear","D) Partial"},
     1,"RHS=0 (no forcing/free term) makes it a homogeneous linear ODE.",QQ(TOPIC_MATHS,4)},
    {"For a 2nd-order linear ODE with constant coefficients, the auxiliary equation is obtained by substituting:",
     {"A) y=x^m","B) y=e^(mx)","C) y=sin(mx)","D) y=ln(mx)"},
     1,"Substituting y=e^(mx) converts the ODE into an algebraic equation in m (the auxiliary equation).",QQ(TOPIC_MATHS,4)},
    {"If the auxiliary equation has real, distinct roots m1, m2, the complementary function is:",
     {"A) C1*e^(m1x) + C2*e^(m2x)","B) (C1+C2x)e^(m1x)","C) e^(m1x)(C1cos(m2x)+C2sin(m2x))","D) C1*x^m1"},
     0,"Distinct real roots give a CF that is a sum of independent exponentials.",QQ(TOPIC_MATHS,4)},
    {"If the auxiliary equation has equal (repeated) roots m, m, the complementary function is:",
     {"A) C1*e^(mx) + C2*e^(mx)","B) (C1 + C2*x)*e^(mx)","C) C1*e^(mx)*C2*e^(mx)","D) C1*x + C2"},
     1,"Repeated roots require multiplying the second solution by x: (C1+C2x)e^(mx).",QQ(TOPIC_MATHS,4)},
    {"The general solution of a linear non-homogeneous ODE equals:",
     {"A) Only the particular integral (PI)","B) Complementary function (CF) plus particular integral (PI)","C) Only the complementary function (CF)","D) CF minus PI"},
     1,"y = CF + PI: CF solves the homogeneous part, PI is any solution matching the RHS.",QQ(TOPIC_MATHS,4)},
    {"1/F(D) operating on e^(ax), when F(a) != 0, gives:",
     {"A) e^(ax)/F(a)","B) x*e^(ax)/F'(a)","C) 0","D) F(a)*e^(ax)"},
     0,"Standard shift-rule result: substitute D=a directly, as long as F(a) is nonzero.",QQ(TOPIC_MATHS,4)},
    {"The particular integral for RHS = sin(ax) or cos(ax) is found by substituting D^2 = ___ in F(D):",
     {"A) a","B) a^2","C) -a^2","D) 2a"},
     2,"Standard rule: replace D^2 with -a^2 when the RHS involves sin(ax) or cos(ax), provided F(-a^2)!=0.",QQ(TOPIC_MATHS,4)},
    {"The method of variation of parameters is generally used to find the particular integral when:",
     {"A) The RHS is a simple polynomial only","B) Undetermined-coefficient/operator shortcuts don't easily apply (e.g. RHS = tan x)","C) The equation is homogeneous","D) There are no boundary conditions"},
     1,"Variation of parameters handles general RHS functions where simple operator methods fail.",QQ(TOPIC_MATHS,4)},
    {"Two solutions y1, y2 of a homogeneous linear ODE are linearly independent if their Wronskian W(y1,y2) is:",
     {"A) Zero everywhere","B) Nonzero (at least at one point)","C) Negative","D) Equal to y1*y2"},
     1,"A nonzero Wronskian confirms the solutions are linearly independent, forming a valid fundamental set.",QQ(TOPIC_MATHS,4)},
    {"The superposition principle for linear homogeneous ODEs states that:",
     {"A) Only one solution can exist","B) If y1 and y2 are solutions, then C1*y1+C2*y2 is also a solution","C) Solutions cannot be added","D) The equation must be nonlinear"},
     1,"Linearity guarantees any linear combination of solutions is itself a solution.",QQ(TOPIC_MATHS,4)},
    {"For the ODE (D^2 - 5D + 6)y = 0, the auxiliary equation roots are:",
     {"A) 2, 3","B) 1, 6","C) -2, -3","D) 5, 6"},
     0,"m^2-5m+6=0 factors as (m-2)(m-3)=0, giving roots m=2 and m=3.",QQ(TOPIC_MATHS,4)},
    {"A differential equation is called linear if the dependent variable and its derivatives appear:",
     {"A) Raised to powers other than 1, or inside functions like sin(y)","B) Only to the first power, and are not multiplied together","C) Multiplied by each other","D) As exponents"},
     1,"Linearity requires y and its derivatives to appear only to the first power, with no products among them.",QQ(TOPIC_MATHS,4)},
    {"d/dx(x^5) =",
     {"A) 5x^4","B) x^4","C) 5x^6","D) x^6/6"},
     0,"Power rule: d/dx(x^n)=n*x^(n-1). So d/dx(x^5)=5x^4.",QQ(TOPIC_MATHS,0)},
    /* Unit 5: Numerical Methods */
    {"The Newton-Raphson method for finding roots uses the iterative formula:",
     {"A) x_(n+1) = x_n - f(x_n)/f'(x_n)","B) x_(n+1) = x_n + f(x_n)","C) x_(n+1) = (x_n + f(x_n))/2","D) x_(n+1) = f(x_n)/x_n"},
     0,"Newton-Raphson: x_(n+1) = x_n - f(x_n)/f'(x_n), using the tangent line to approach the root.",QQ(TOPIC_MATHS,5)},
    {"The Bisection method for root finding requires that f(a) and f(b):",
     {"A) Have the same sign","B) Have opposite signs (so a root lies between a and b)","C) Both equal zero","D) Be equal to each other"},
     1,"Bisection relies on the Intermediate Value Theorem: f(a)*f(b)<0 guarantees a root in (a,b).",QQ(TOPIC_MATHS,5)},
    {"Compared to the Bisection method, the Newton-Raphson method generally converges:",
     {"A) Slower (linearly)","B) Faster (quadratically), when it converges","C) Never","D) At the same rate always"},
     1,"Newton-Raphson has quadratic convergence near a simple root, much faster than bisection's linear convergence.",QQ(TOPIC_MATHS,5)},
    {"The Regula-Falsi (False Position) method differs from Bisection because it:",
     {"A) Uses the derivative of f","B) Uses linear interpolation between the endpoints to estimate the root","C) Requires f(a) and f(b) to have the same sign","D) Cannot be used for continuous functions"},
     1,"False position estimates the root via linear interpolation between (a,f(a)) and (b,f(b)) rather than simply bisecting.",QQ(TOPIC_MATHS,5)},
    {"Newton's forward difference interpolation formula is most suitable when the value to be interpolated is near:",
     {"A) The end of the data table","B) The beginning of the data table","C) The middle of the data table","D) Outside the data range"},
     1,"Forward differences are built from the start of the table, so forward interpolation works best near the beginning.",QQ(TOPIC_MATHS,5)},
    {"Lagrange's interpolation formula, unlike Newton's finite-difference formulas, requires the data points to be:",
     {"A) Equally spaced","B) Not necessarily equally spaced","C) Exactly three in number","D) Sorted in decreasing order"},
     1,"Lagrange interpolation works for arbitrarily spaced data points, unlike finite-difference-based Newton formulas.",QQ(TOPIC_MATHS,5)},
    {"The Trapezoidal rule approximates the area under a curve using:",
     {"A) Rectangles","B) Straight-line (trapezoid) segments between points","C) Parabolic segments","D) Circular arcs"},
     1,"Trapezoidal rule connects consecutive points with straight lines, summing trapezoid areas.",QQ(TOPIC_MATHS,5)},
    {"Simpson's 1/3 rule approximates the curve between points using:",
     {"A) Straight lines","B) Parabolic (quadratic) arcs, requiring an even number of intervals","C) Cubic curves","D) Circular arcs"},
     1,"Simpson's 1/3 rule fits a parabola through each set of 3 points, requiring an even number of subintervals.",QQ(TOPIC_MATHS,5)},
    {"Simpson's 3/8 rule requires the number of intervals to be a multiple of:",
     {"A) 2","B) 3","C) 4","D) 5"},
     1,"Simpson's 3/8 rule fits a cubic through 4 points, so the number of intervals must be a multiple of 3.",QQ(TOPIC_MATHS,5)},
    {"Euler's method for solving ODEs numerically uses the update formula:",
     {"A) y_(n+1) = y_n + h*f(x_n, y_n)","B) y_(n+1) = y_n - h*f(x_n,y_n)","C) y_(n+1) = y_n * h","D) y_(n+1) = f(x_n)"},
     0,"Euler's method takes a linear step using the slope f(x_n,y_n) at the current point, with step size h.",QQ(TOPIC_MATHS,5)},
    {"The 4th order Runge-Kutta (RK4) method is generally preferred over Euler's method because it:",
     {"A) Requires only one function evaluation","B) Gives significantly higher accuracy for a given step size","C) Cannot handle nonlinear equations","D) Ignores the step size"},
     1,"RK4 uses 4 weighted slope estimates per step, achieving much higher accuracy than Euler's 1st-order method.",QQ(TOPIC_MATHS,5)},
    {"Gauss elimination solves a system of linear equations by:",
     {"A) Guessing solutions randomly","B) Reducing the augmented matrix to upper-triangular form via row operations","C) Using derivatives","D) Using only matrix multiplication"},
     1,"Gauss elimination systematically eliminates variables using row operations, then back-substitutes.",QQ(TOPIC_MATHS,5)},
    {"In numerical methods, truncation error arises mainly due to:",
     {"A) Computer rounding of decimals","B) Approximating an infinite/exact process with a finite one","C) Hardware failure","D) Input typos"},
     1,"Truncation error comes from approximating a mathematical procedure (like an infinite series or exact derivative) by a finite process.",QQ(TOPIC_MATHS,5)},
    {"Round-off error in numerical computation arises because:",
     {"A) The method itself is wrong","B) A computer can only store numbers with finite precision","C) The function is discontinuous","D) The interval is too large"},
     1,"Round-off error results from the finite precision (limited digits) used in computer arithmetic.",QQ(TOPIC_MATHS,5)},
    {"For an iterative method x_(n+1)=g(x_n) to converge, a sufficient condition near the root is:",
     {"A) |g'(x)| > 1","B) |g'(x)| < 1","C) g'(x) = 0 always","D) g(x) is discontinuous"},
     1,"Fixed-point iteration converges if |g'(x)|<1 near the root (contraction mapping condition).",QQ(TOPIC_MATHS,5)}
};

static const QuizQuestion AI_QUIZ[] = {
    {"Which ML type uses labelled data?",
     {"A) Unsupervised","B) Reinforcement","C) Supervised","D) Generative"},
     2,"Supervised learning trains on labelled input-output pairs.",QQ(TOPIC_AI,0)},
    {"ReLU activation function is:",
     {"A) 1/(1+e^-x)","B) max(0,x)","C) tanh(x)","D) x^2"},
     1,"ReLU = max(0,x). Outputs 0 for negative inputs, x for positive.",QQ(TOPIC_AI,0)},
    {"A* search uses f(n) =",
     {"A) h(n)","B) g(n)","C) g(n)+h(n)","D) g(n)*h(n)"},
     2,"A*: f(n)=g(n)+h(n). g=cost so far, h=heuristic estimate to goal.",QQ(TOPIC_AI,0)},
    {"Overfitting causes:",
     {"A) High train + high test error","B) Low train + high test error","C) Low train + low test error","D) High train + low test error"},
     1,"Overfitting: memorises training data => low train error, high test error.",QQ(TOPIC_AI,0)},
    {"PCA reduces dimensionality by finding directions of:",
     {"A) Minimum variance","B) Maximum variance","C) Zero variance","D) Median variance"},
     1,"PCA finds principal components = directions of maximum variance in data.",QQ(TOPIC_AI,0)}
};

static const QuizQuestion CPROG_QUIZ[] = {
    /* Unit 1: Introduction / Logical Reasoning & Algorithms */
    {"C language was developed by:",
     {"A) Bjarne Stroustrup","B) Dennis Ritchie","C) James Gosling","D) Guido van Rossum"},
     1,"Dennis Ritchie developed C at Bell Labs (1972). Used to write UNIX.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"Which header file is needed for printf and scanf?",
     {"A) stdlib.h","B) string.h","C) stdio.h","D) math.h"},
     2,"stdio.h (standard input/output) contains printf, scanf, fgets, puts, etc.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"The entry point of every C program is:",
     {"A) start()","B) begin()","C) init()","D) main()"},
     3,"Every C program must have a main() function; execution begins there.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"An algorithm is best described as:",
     {"A) A programming language","B) A step-by-step procedure to solve a problem","C) A type of variable","D) A compiler"},
     1,"An algorithm is a finite, well-defined sequence of steps to solve a problem, independent of any language.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"A flowchart is used to:",
     {"A) Compile code faster","B) Visually represent the logic/flow of an algorithm","C) Store variables","D) Execute a program"},
     1,"Flowcharts use standard symbols to visually depict the sequence of steps in an algorithm.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"Which is a valid identifier in C?",
     {"A) 2value","B) _value","C) value-1","D) int"},
     1,"Identifiers must start with a letter or underscore; digits-first, hyphens, and reserved keywords are invalid.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"C is classified as a:",
     {"A) High-level, procedural language","B) Purely object-oriented language","C) Markup language","D) Scripting-only language"},
     0,"C is a general-purpose, procedural (structured) high-level programming language.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"The C compiler converts source code into:",
     {"A) Machine code (executable)","B) Another high-level language","C) Plain text","D) A flowchart"},
     0,"The compiler translates human-readable C source into machine code the CPU can execute.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"In C, all simple statements must end with a:",
     {"A) Colon (:)","B) Semicolon (;)","C) Period (.)","D) Comma (,)"},
     1,"The semicolon terminates simple statements in C.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"Which data type is used to store a single character in C?",
     {"A) int","B) char","C) float","D) string"},
     1,"char stores a single character (1 byte), typically using its ASCII value.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"The size of an int on most modern systems is typically:",
     {"A) 1 byte","B) 2 bytes","C) 4 bytes","D) 8 bytes"},
     2,"int is commonly 4 bytes on most modern compilers/systems (the C standard doesn't fix this exactly).",QQ(TOPIC_C_PROGRAMMING,1)},
    {"Which operator has the highest precedence in C?",
     {"A) + (addition)","B) () (parentheses/function call)","C) = (assignment)","D) || (logical OR)"},
     1,"Parentheses (grouping/function call) have the highest precedence, evaluated first.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"Time complexity of an algorithm measures:",
     {"A) The memory it uses","B) How its running time grows with input size","C) The number of lines of code","D) The programming language used"},
     1,"Time complexity (e.g. Big-O) expresses how execution time scales as input size grows.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"Pseudocode is:",
     {"A) Actual compilable code","B) An informal, high-level description of an algorithm's logic","C) A type of compiler error","D) A binary file"},
     1,"Pseudocode describes algorithm logic in plain, structured language without strict syntax rules.",QQ(TOPIC_C_PROGRAMMING,1)},
    {"Which of these is NOT a valid C keyword?",
     {"A) return","B) integer","C) while","D) static"},
     1,"'integer' is not a C keyword; the correct type keyword is 'int'.",QQ(TOPIC_C_PROGRAMMING,1)},
    /* Unit 2: Strings & Functions / Decision Controls & Loops */
    {"strlen(\"Hello\") returns:",
     {"A) 6","B) 5","C) 4","D) 0"},
     1,"strlen counts characters up to but NOT including '\\0'. \"Hello\" has 5 chars.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"static local variable:",
     {"A) Makes it const","B) Stores in register","C) Persists between function calls","D) Makes it global"},
     2,"static local: initialised once; retains value between function calls.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"Recursion MUST have a:",
     {"A) Return type void","B) Base case","C) Global variable","D) Loop inside"},
     1,"Without a base case, recursion never stops and causes stack overflow.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"The 'if-else' statement in C is used for:",
     {"A) Repeating a block of code","B) Choosing between two alternative code paths","C) Declaring variables","D) Defining functions"},
     1,"if-else selects one of two branches to execute based on a condition.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"Which loop in C is guaranteed to execute its body at least once?",
     {"A) for","B) while","C) do-while","D) None of these"},
     2,"do-while checks the condition AFTER executing the body, so it always runs at least once.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"The 'switch' statement in C is best suited for:",
     {"A) Testing a single variable against many discrete constant values","B) Looping indefinitely","C) Declaring arrays","D) Allocating memory"},
     0,"switch efficiently branches based on matching a variable to one of several constant case values.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"Without a 'break' statement, execution in a switch-case:",
     {"A) Stops immediately after a matching case","B) 'Falls through' into subsequent cases","C) Causes a compile error","D) Skips the entire switch"},
     1,"Without break, control flows ('falls through') into the next case's statements.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"The 'for' loop's three parts (init; condition; update) work as:",
     {"A) update, then condition, then init","B) init once, then condition tested, then update after each iteration","C) All optional and must be blank","D) Only usable with integers"},
     1,"init runs once at the start; condition is checked before each iteration; update runs after each iteration's body.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"The 'continue' statement in a loop:",
     {"A) Terminates the loop entirely","B) Skips the rest of the current iteration and moves to the next","C) Restarts the program","D) Pauses execution"},
     1,"continue skips remaining code in the current iteration and jumps to the loop's next iteration check.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"The 'break' statement, when used inside a loop:",
     {"A) Skips to the next iteration","B) Immediately exits the innermost loop","C) Exits the entire program","D) Restarts the loop"},
     1,"break immediately terminates the nearest enclosing loop or switch statement.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"What does the ternary operator '?:' do in C?",
     {"A) Declares a pointer","B) Provides a compact conditional expression","C) Performs a loop","D) Allocates memory"},
     1,"The ternary operator is shorthand for: (condition) ? value_if_true : value_if_false.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"An infinite loop in C can be intentionally created using:",
     {"A) for(;;)","B) if(1)","C) switch(1)","D) return;"},
     0,"for(;;) with no condition runs forever unless broken out of explicitly.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"In C, any nonzero value used as an if condition is treated as:",
     {"A) False","B) True","C) A syntax error","D) Undefined"},
     1,"C has no separate boolean type historically; zero is false, any nonzero value is true.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"Which logical operator returns true only if BOTH operands are true?",
     {"A) ||","B) &&","C) !","D) ^"},
     1,"&& is the logical AND operator, true only when both operands are true.",QQ(TOPIC_C_PROGRAMMING,2)},
    {"Which of these is a valid relational operator in C?",
     {"A) =<","B) >=","C) =!","D) <>"},
     1,">= (greater than or equal to) is valid C syntax; =<, =!, and <> are not.",QQ(TOPIC_C_PROGRAMMING,2)},
    /* Unit 3: 2D Arrays / Arrays & Strings */
    {"int a[3][4] has how many elements?",
     {"A) 7","B) 3","C) 4","D) 12"},
     3,"Total elements = rows * cols = 3 * 4 = 12.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"C stores 2D arrays in:",
     {"A) Column-major order","B) Row-major order","C) Random order","D) Diagonal order"},
     1,"C uses row-major order: all elements of row 0, then row 1, etc.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"In C, array indexing starts at:",
     {"A) 1","B) 0","C) -1","D) Depends on the compiler"},
     1,"C arrays are zero-indexed; the first element is at index 0.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"A string in C is represented as:",
     {"A) A special 'string' data type","B) A character array terminated by '\\0'","C) An integer array","D) A struct"},
     1,"C has no built-in string type; strings are char arrays ending with the null terminator.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"Which function is used to copy one string into another in C?",
     {"A) strcpy()","B) strcat()","C) strcmp()","D) strlen()"},
     0,"strcpy(dest, src) copies the src string (including the null terminator) into dest.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"strcat() is used to:",
     {"A) Compare two strings","B) Copy a string","C) Append (concatenate) one string onto another","D) Find string length"},
     2,"strcat(dest, src) appends src onto the end of dest.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"strcmp(s1, s2) returns 0 when:",
     {"A) s1 is longer than s2","B) The two strings are identical","C) s1 is shorter than s2","D) Never"},
     1,"strcmp returns 0 only if the two strings are exactly equal, character-for-character.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"Passing an array to a function in C actually passes:",
     {"A) A full copy of the array","B) A pointer to the array's first element","C) Nothing","D) Only the last element"},
     1,"Arrays decay to a pointer to their first element when passed to functions.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"To declare a 2D array of 3 rows and 4 columns, you write:",
     {"A) int a[3,4];","B) int a[3][4];","C) int a(3,4);","D) int a{3}{4};"},
     1,"C uses separate bracket pairs for each dimension: int a[rows][cols].",QQ(TOPIC_C_PROGRAMMING,3)},
    {"Given int a[5] = {1,2,3}, the remaining elements a[3] and a[4] are:",
     {"A) Garbage/undefined values","B) Automatically initialised to 0","C) Equal to a[0]","D) A compile error"},
     1,"Partially initialised arrays have their remaining elements automatically zero-initialised in C.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"The array name 'a' by itself (without an index) represents:",
     {"A) The value of the first element","B) The address of the first element (a pointer)","C) The total number of elements","D) A syntax error"},
     1,"In most expressions, the array name decays to a pointer to its first element.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"Which header file provides string handling functions like strlen and strcpy?",
     {"A) stdio.h","B) stdlib.h","C) string.h","D) ctype.h"},
     2,"string.h declares strlen, strcpy, strcat, strcmp, and other string functions.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"A string literal like \"Hello\" occupies how many bytes in memory?",
     {"A) 5","B) 6 (includes the null terminator)","C) 4","D) 10"},
     1,"The compiler automatically appends '\\0', so \"Hello\" needs 5+1=6 bytes.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"gets() is considered unsafe compared to fgets() mainly because gets():",
     {"A) Is slower","B) Does not check buffer bounds, risking buffer overflow","C) Cannot read strings","D) Requires a file pointer"},
     1,"gets() has no way to limit input length, making it prone to buffer overflows; fgets() takes a size limit.",QQ(TOPIC_C_PROGRAMMING,3)},
    {"The index of the last valid element in an array declared as int a[10] is:",
     {"A) 10","B) 9","C) 11","D) 0"},
     1,"With 10 elements indexed 0-9, the last valid index is 9 (a[10] would be out of bounds).",QQ(TOPIC_C_PROGRAMMING,3)},
    /* Unit 4: Structures & Pointers / Functions, Structures & Pointers */
    {"Struct member access via pointer uses:",
     {"A) .","B) ->","C) *","D) &"},
     1,"Arrow operator (->) accesses struct member through pointer: ptr->member.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"A function prototype in C is used to:",
     {"A) Define the function's full body","B) Declare a function's name, return type, and parameters before its use","C) Allocate memory","D) Import a library"},
     1,"A prototype tells the compiler the function's signature ahead of its actual definition/use.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"Parameters in C are passed to functions, by default, by:",
     {"A) Reference","B) Value (a copy is passed)","C) Pointer only","D) Global reference"},
     1,"C uses pass-by-value by default; the function receives a copy of the argument.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"To modify a caller's variable from inside a function, you must pass:",
     {"A) The value directly","B) A pointer to that variable","C) A string","D) Nothing; it's impossible in C"},
     1,"Passing the variable's address (a pointer) lets the function dereference and modify the original.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"A pointer variable in C stores:",
     {"A) A copy of a value","B) The memory address of another variable","C) A function name","D) A file path"},
     1,"A pointer holds the memory address where a value is stored, not the value itself.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"The '&' operator in C, when applied to a variable, returns its:",
     {"A) Value","B) Address","C) Size","D) Type"},
     1,"The address-of operator '&' yields the memory address of its operand.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"The '*' operator, when applied to a pointer, is called the ___ operator:",
     {"A) Address-of","B) Dereference","C) Modulus","D) Multiplication"},
     1,"The unary '*' dereferences a pointer, accessing the value stored at the address it holds.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"A struct in C is used to:",
     {"A) Group variables of different types under one name","B) Create a loop","C) Define a single integer","D) Import a header"},
     0,"struct groups related variables (possibly of different types) into a single user-defined type.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"Struct member access via a variable (not a pointer) uses the operator:",
     {"A) ->","B) . (dot)","C) &","D) *"},
     1,"The dot operator accesses a member directly on a struct variable/instance.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"A function that calls itself, directly or indirectly, is called:",
     {"A) An iterative function","B) A recursive function","C) A static function","D) A void function"},
     1,"Recursion occurs when a function calls itself to solve smaller instances of a problem.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"The scope of a variable declared inside a function (without 'static') is:",
     {"A) Global to the whole program","B) Local to that function only","C) Accessible from any file","D) Permanent for the program's lifetime"},
     1,"Local (automatic) variables exist only within the function they're declared in.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"A void pointer (void*) in C:",
     {"A) Cannot hold any address","B) Is a generic pointer that can point to any data type","C) Is the same as a NULL pointer","D) Can only point to integers"},
     1,"void* is a type-agnostic pointer, commonly used for generic functions like malloc().",QQ(TOPIC_C_PROGRAMMING,4)},
    {"Pointer arithmetic (e.g. ptr+1) advances the pointer by:",
     {"A) Exactly 1 byte, always","B) sizeof(the pointed-to type) bytes","C) A random amount","D) 0 bytes"},
     1,"Incrementing a pointer moves it forward by the size of the type it points to, not just 1 byte.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"A function declared with return type 'void':",
     {"A) Must return an integer","B) Returns no value","C) Cannot take parameters","D) Is always recursive"},
     1,"void as a return type means the function does not return any value to the caller.",QQ(TOPIC_C_PROGRAMMING,4)},
    {"An array of structures is used when you need to:",
     {"A) Store multiple records, each with the same set of fields","B) Store a single primitive value","C) Avoid using pointers","D) Replace loops entirely"},
     0,"An array of structs stores many records (e.g. multiple students) sharing the same field layout.",QQ(TOPIC_C_PROGRAMMING,4)},
    /* Unit 5: Dynamic Memory / Dynamic Memory & Linked Lists */
    {"calloc() vs malloc(): calloc also:",
     {"A) Is faster","B) Zero-initialises memory","C) Allocates on stack","D) Takes one argument"},
     1,"calloc(n,size) allocates n*size bytes AND zero-initialises all bytes.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"free() in C is used to:",
     {"A) Release stack memory","B) Release heap memory","C) Clear a variable","D) Reset a pointer to NULL"},
     1,"free() releases memory previously allocated by malloc/calloc/realloc on the heap.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"malloc() allocates memory on the:",
     {"A) Stack","B) Heap","C) Register","D) Read-only segment"},
     1,"malloc/calloc/realloc all allocate from the heap, which persists until explicitly freed.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"malloc(n) returns:",
     {"A) A void pointer to n bytes of uninitialised memory, or NULL on failure","B) An initialised array","C) A struct","D) An integer value n"},
     0,"malloc reserves n bytes without initialising them, returning NULL if allocation fails.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"Failing to free() dynamically allocated memory that's no longer needed causes:",
     {"A) A compile error","B) A memory leak","C) Automatic garbage collection","D) An immediate segmentation fault"},
     1,"Unreleased heap memory that's no longer reachable is a memory leak, wasting resources over time.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"realloc(ptr, newsize) is used to:",
     {"A) Free memory immediately","B) Resize a previously allocated memory block","C) Allocate memory on the stack","D) Copy a string"},
     1,"realloc adjusts the size of an existing dynamically allocated block, preserving its contents where possible.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"Dereferencing a pointer after it has been freed (a 'dangling pointer') results in:",
     {"A) Guaranteed safe behaviour","B) Undefined behaviour","C) An automatic compiler warning that halts the program","D) The memory being refreshed"},
     1,"Using a pointer after its memory is freed is undefined behaviour — it may crash or corrupt data.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"A singly linked list node typically contains:",
     {"A) Only data","B) Data and a pointer to the next node","C) Only a pointer","D) Two pointers and no data"},
     1,"Each node stores its data plus a pointer to the next node in the list.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"Compared to arrays, linked lists allow:",
     {"A) Faster random access","B) Dynamic size, with efficient insertion/deletion (no shifting)","C) No use of pointers","D) Fixed size only"},
     1,"Linked lists grow/shrink dynamically and can insert/delete nodes without shifting other elements.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"In a (non-circular) linked list, the last node's 'next' pointer is typically set to:",
     {"A) The first node (always)","B) NULL","C) Itself","D) A random address"},
     1,"NULL marks the end of a non-circular linked list.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"Compared to a singly linked list, a doubly linked list node additionally has:",
     {"A) A pointer to the previous node","B) Two data fields","C) No pointers","D) A fixed size array"},
     0,"Doubly linked list nodes store pointers to both the next AND previous nodes.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"calloc(n, size), compared to malloc(n*size), additionally:",
     {"A) Is always faster","B) Zero-initialises all allocated bytes","C) Doesn't need to be freed","D) Only allocates on the stack"},
     1,"calloc guarantees the allocated memory starts zeroed out, unlike malloc which leaves it uninitialised.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"A memory leak specifically occurs when:",
     {"A) A program uses too many local variables","B) Dynamically allocated memory becomes unreachable without being freed","C) A pointer is set to NULL","D) An array is declared too large"},
     1,"Once the last pointer to a heap block is lost without calling free(), that memory can never be reclaimed.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"Which function releases memory previously obtained via malloc/calloc/realloc?",
     {"A) delete()","B) free()","C) release()","D) clear()"},
     1,"free() is the standard C function to deallocate heap memory obtained from malloc/calloc/realloc.",QQ(TOPIC_C_PROGRAMMING,5)},
    {"Before using a pointer returned by malloc(), it is good practice to check if it is:",
     {"A) Equal to 0 (NULL), indicating allocation failure","B) Negative","C) A string","D) Equal to the heap size"},
     0,"malloc returns NULL if allocation fails (e.g., out of memory); checking for NULL avoids dereferencing an invalid pointer.",QQ(TOPIC_C_PROGRAMMING,5)}
};

static const QuizQuestion CHEMISTRY_QUIZ[] = {
    {"Avogadro's number:",
     {"A) 6.022e23","B) 6.022e20","C) 3.14e23","D) 9.8e23"},
     0,"1 mole = 6.022 x 10^23 particles.",QQ(TOPIC_CHEMISTRY,0)},
    {"pH=3 means solution is:",
     {"A) Basic","B) Neutral","C) Acidic","D) Amphoteric"},
     2,"pH < 7 = acidic. pH=3 = strongly acidic ([H+]=10^-3 mol/L).",QQ(TOPIC_CHEMISTRY,0)},
    {"Oxidation means:",
     {"A) Gain of electrons","B) Loss of electrons","C) Gain of protons","D) Loss of protons"},
     1,"OIL RIG: Oxidation Is Loss (of electrons). Reduction Is Gain.",QQ(TOPIC_CHEMISTRY,0)},
    {"Ideal Gas Law:",
     {"A) PV=nT","B) P/V=nR","C) PV=nRT","D) PT=nRV"},
     2,"PV=nRT. P=pressure, V=volume, n=moles, R=8.314 J/mol/K, T=Kelvin.",QQ(TOPIC_CHEMISTRY,0)},
    {"Ionic bond involves:",
     {"A) Equal electron sharing","B) Unequal sharing","C) Electron transfer metal->non-metal","D) Electron transfer non-metal->metal"},
     2,"Ionic bond: metal TRANSFERS electrons to non-metal. Example: Na->Cl in NaCl.",QQ(TOPIC_CHEMISTRY,0)}
};

static const QuizQuestion ELECTRONICS_QUIZ[] = {
    {"Forward voltage drop of silicon diode:",
     {"A) 0V","B) 0.3V","C) 0.7V","D) 1.4V"},
     2,"Silicon diode forward voltage ~0.7V. Germanium ~0.3V.",QQ(TOPIC_ELECTRONICS,0)},
    {"In BJT, IC = beta * IB. Beta is:",
     {"A) Base voltage","B) Current gain","C) Collector resistance","D) Emitter current"},
     1,"Beta (hFE) = DC current gain of transistor: IC=beta*IB.",QQ(TOPIC_ELECTRONICS,0)},
    {"Universal logic gate:",
     {"A) AND","B) OR","C) XOR","D) NAND"},
     3,"NAND (and NOR) are universal gates — any logic function can be built from them.",QQ(TOPIC_ELECTRONICS,0)},
    {"PWM duty cycle 25% with 12V supply gives average output:",
     {"A) 12V","B) 6V","C) 3V","D) 9V"},
     2,"V_avg = duty_cycle * Vsupply = 0.25 * 12 = 3V.",QQ(TOPIC_ELECTRONICS,0)},
    {"555 timer in astable mode acts as:",
     {"A) One-shot pulse generator","B) SR flip-flop","C) Free-running oscillator","D) Voltage regulator"},
     2,"Astable mode: no stable state; oscillates continuously as square wave generator.",QQ(TOPIC_ELECTRONICS,0)}
};

/* ================================================================== */
/*  get_quiz_question and get_quiz_count — with unit filtering        */
/* ================================================================== */

/*
 * get_all_quiz — returns the base array and size for a topic.
 * Internal helper used by the two public functions.
 */
static const QuizQuestion *get_topic_quiz(SubjectTopic topic, int *total_out)
{
    switch (topic) {
        case TOPIC_PHYSICS:
            *total_out = (int)(sizeof(PHYSICS_QUIZ)/sizeof(PHYSICS_QUIZ[0]));
            return PHYSICS_QUIZ;
        case TOPIC_CIVIL_ENGINEERING:
            *total_out = (int)(sizeof(CIVIL_QUIZ)/sizeof(CIVIL_QUIZ[0]));
            return CIVIL_QUIZ;
        case TOPIC_MATHS:
            *total_out = (int)(sizeof(MATHS_QUIZ)/sizeof(MATHS_QUIZ[0]));
            return MATHS_QUIZ;
        case TOPIC_AI:
            *total_out = (int)(sizeof(AI_QUIZ)/sizeof(AI_QUIZ[0]));
            return AI_QUIZ;
        case TOPIC_C_PROGRAMMING:
            *total_out = (int)(sizeof(CPROG_QUIZ)/sizeof(CPROG_QUIZ[0]));
            return CPROG_QUIZ;
        case TOPIC_CHEMISTRY:
            *total_out = (int)(sizeof(CHEMISTRY_QUIZ)/sizeof(CHEMISTRY_QUIZ[0]));
            return CHEMISTRY_QUIZ;
        case TOPIC_ELECTRONICS:
            *total_out = (int)(sizeof(ELECTRONICS_QUIZ)/sizeof(ELECTRONICS_QUIZ[0]));
            return ELECTRONICS_QUIZ;
        default: *total_out = 0; return NULL;
    }
}

int get_quiz_question(SubjectTopic topic, int unit, int index, QuizQuestion *q_out)
{
    int total = 0, nth = 0, i;
    const QuizQuestion *arr = get_topic_quiz(topic, &total);
    if (!arr || !q_out) return 0;
    for (i = 0; i < total; i++) {
        /* include if unit==0 (all) OR question's unit==0 OR question's unit matches */
        if (unit == 0 || arr[i].unit == 0 || arr[i].unit == unit) {
            if (nth == index) { *q_out = arr[i]; return 1; }
            nth++;
        }
    }
    return 0;
}

int get_quiz_count(SubjectTopic topic, int unit)
{
    int total = 0, count = 0, i;
    const QuizQuestion *arr = get_topic_quiz(topic, &total);
    if (!arr) return 0;
    for (i = 0; i < total; i++) {
        if (unit == 0 || arr[i].unit == 0 || arr[i].unit == unit)
            count++;
    }
    return count > 0 ? count : total; /* fallback: show all if none match */
}
