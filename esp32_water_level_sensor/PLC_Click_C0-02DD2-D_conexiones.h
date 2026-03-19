/*
 * ============================================================================
 *  PLC Click C0-02DD2-D + Sensor ALS-MPM-2F (4-20 mA)
 * ============================================================================
 *
 *  PLC: AutomationDirect Click C0-02DD2-D
 *    - CPU con 8 DI / 6 DO (discretas, 24VDC)
 *    - NO tiene entradas analógicas integradas
 *    - Requiere módulo analógico de expansión
 *
 *  Módulo requerido: C0-04AD-1 (4 canales, 4-20 mA / 0-10V)
 *    - Resolución: 12 bits (0-4095)
 *    - Se instala en cualquier slot de expansión del PLC
 *
 *  Sensor: ALS-MPM-2F
 *    - Salida: 4-20 mA
 *    - Alimentación: 24 VDC
 *    - Rango: 0 - 5 metros
 *
 * ============================================================================
 *  DIAGRAMA DE CONEXIONES - PLC CLICK
 * ============================================================================
 *
 *                         MÓDULO ANALÓGICO
 *   +----------+          C0-04AD-1                +------------------+
 *   | Fuente   |       +------------------+        | PLC Click        |
 *   | 24 VDC   |       |  Canal 1 (CH1)   |        | C0-02DD2-D       |
 *   |          |       |                  |        |                  |
 *   |          |       |  I1+ ○───────┐   |        |  Slot Expansión  |
 *   |  (+24V)--+--+--->|  (Terminal 1)|   |◄══════►|  (Ribbon Cable)  |
 *   |          |  |    |              |   |        |                  |
 *   |          |  |    |  I1- ○───┐   |   |        |   C1 = Puerto COM|
 *   |          |  |    |  (Term 2)|   |   |        |   Puerto USB     |
 *   |          |  |    +----------|---|---+        +------------------+
 *   |          |  |               |   |
 *   |          |  |    +----------|---|------------+
 *   |          |  |    | Sensor ALS-MPM-2F        |
 *   |          |  |    |          |   |            |
 *   |          |  +--->| Vcc(+)───┘   |            |
 *   |          |       |              |            |
 *   |          |       | Signal(-)────┘            |
 *   |          |       |    (Salida 4-20 mA)       |
 *   |   (GND)--+------| GND                       |
 *   +----------+       +---------------------------+
 *
 *
 * ============================================================================
 *  CONEXIÓN DETALLADA - TERMINALES DEL MÓDULO C0-04AD-1
 * ============================================================================
 *
 *   Módulo C0-04AD-1 (Canal 1 para 4-20 mA):
 *   ┌─────────────────────────────────────┐
 *   │  Terminal    Conexión               │
 *   │  ─────────   ──────────────────     │
 *   │  I1+ (1)     Cable del sensor (+)   │
 *   │  I1- (2)     Cable del sensor (-)   │
 *   │                                     │
 *   │  I2+ (3)     (Canal 2 - libre)      │
 *   │  I2- (4)     (Canal 2 - libre)      │
 *   │                                     │
 *   │  I3+ (5)     (Canal 3 - libre)      │
 *   │  I3- (6)     (Canal 3 - libre)      │
 *   │                                     │
 *   │  I4+ (7)     (Canal 4 - libre)      │
 *   │  I4- (8)     (Canal 4 - libre)      │
 *   └─────────────────────────────────────┘
 *
 *   IMPORTANTE: El módulo C0-04AD-1 ya incluye la resistencia
 *   de precisión interna para la conversión de 4-20 mA.
 *   NO se necesita resistencia externa (a diferencia del ESP32).
 *
 *
 * ============================================================================
 *  CIRCUITO COMPLETO CON PLC + ESP32 (OPCIONAL)
 * ============================================================================
 *
 *   Si se desea usar AMBOS (PLC para control + ESP32 para monitoreo IoT):
 *
 *                  +---[ Sensor ALS-MPM-2F ]---+
 *                  |     (Lazo 4-20 mA)        |
 *                  |                           |
 *    +----------+  |   +--------+              |
 *    | Fuente   +--+-->| C0-04AD|──► PLC Click |
 *    | 24 VDC   |  |   | (CH1)  |   (Control)  |
 *    |          |  |   +--------+              |
 *    |          |  |                           |
 *    |          |  |   +--------+              |
 *    |          |  +-->| 150 Ω  |──► ESP32     |
 *    |          |      | (shunt)|   (IoT/WiFi) |
 *    |   GND ---+------+--------+---► GND      |
 *    +----------+                              |
 *                                              |
 *   NOTA: En esta configuración el sensor alimenta DOS cargas             |
 *   en serie. Verificar que la tensión de lazo sea suficiente.
 *
 *
 * ============================================================================
 *  CONFIGURACIÓN EN SOFTWARE - Click Programming (CLICK EDIT)
 * ============================================================================
 *
 *  1. CONFIGURAR MÓDULO ANALÓGICO:
 *     - Abrir Click Programming Software
 *     - Ir a Setup → I/O Configuration
 *     - Seleccionar el slot donde está el C0-04AD-1
 *     - Canal 1: Seleccionar "4-20 mA"
 *     - Formato de datos: "Raw" (0-4095) o "Scaled" (con unidades)
 *
 *  2. DIRECCIONES DE MEMORIA:
 *     Si el módulo está en Slot 2:
 *     ┌─────────────────────────────────────────────────┐
 *     │  Dirección    Descripción                       │
 *     │  ──────────   ────────────────────────────────  │
 *     │  AD1          Valor analógico Canal 1 (raw)     │
 *     │               0-4095 → 4-20 mA → 0-5 m         │
 *     │  DF1          Registro flotante para escalado   │
 *     └─────────────────────────────────────────────────┘
 *
 *  3. PROGRAMA LADDER BÁSICO (Pseudocódigo):
 *
 *     ┌──────────────────────────────────────────────────────────────┐
 *     │  RUNG 1: Escalado de señal analógica                        │
 *     │                                                             │
 *     │  ──[SCALE]──────────────────────────────────────            │
 *     │    Input:    AD1        (valor raw 0-4095)                  │
 *     │    In Low:   819        (equivale a 4 mA)                   │
 *     │    In High:  4095       (equivale a 20 mA)                  │
 *     │    Out Low:  0.00       (0 metros)                          │
 *     │    Out High: 5.00       (5 metros)                          │
 *     │    Output:   DF1        (nivel en metros)                   │
 *     ├──────────────────────────────────────────────────────────────┤
 *     │  RUNG 2: Alarma nivel BAJO                                  │
 *     │                                                             │
 *     │  ──[DF1 <= 0.50]────────────────────────(Y1)── Salida Alarma│
 *     ├──────────────────────────────────────────────────────────────┤
 *     │  RUNG 3: Alarma nivel ALTO                                  │
 *     │                                                             │
 *     │  ──[DF1 >= 4.50]────────────────────────(Y2)── Salida Alarma│
 *     ├──────────────────────────────────────────────────────────────┤
 *     │  RUNG 4: Control de bomba (ejemplo)                         │
 *     │                                                             │
 *     │  ──[DF1 <= 1.00]──┬─────────────────────(Y3)── Bomba ON    │
 *     │                   │                                         │
 *     │  ──[Y3]───────────┤  (Enclave/Latch)                       │
 *     │                   │                                         │
 *     │  ──[DF1 >= 4.50]──┘──────────────/──────(Y3)── Bomba OFF   │
 *     ├──────────────────────────────────────────────────────────────┤
 *     │  RUNG 5: Detección de falla del sensor                      │
 *     │                                                             │
 *     │  ──[AD1 < 600]─────────────────────────(Y4)── Falla Sensor  │
 *     │    (menor a ~3.5 mA = cable roto)                           │
 *     └──────────────────────────────────────────────────────────────┘
 *
 *  4. VALORES DE REFERENCIA PARA EL ADC (12 bits, 4-20 mA):
 *     ┌───────────────────────────────────────────┐
 *     │  Corriente   ADC Raw    Nivel    Estado   │
 *     │  ─────────   ───────    ──────   ──────── │
 *     │   < 3.5 mA   < 600     ---      FALLA    │
 *     │   4.0 mA      819      0.00 m   Vacío    │
 *     │   8.0 mA     1638      1.25 m   25%      │
 *     │  12.0 mA     2457      2.50 m   50%      │
 *     │  16.0 mA     3276      3.75 m   75%      │
 *     │  20.0 mA     4095      5.00 m   Lleno    │
 *     └───────────────────────────────────────────┘
 *
 * ============================================================================
 *  LISTA DE MATERIALES
 * ============================================================================
 *
 *  Qty   Parte                    Descripción
 *  ───   ─────────────────────    ──────────────────────────────────
 *   1    C0-02DD2-D               PLC Click CPU (24VDC, 8DI/6DO)
 *   1    C0-04AD-1                Módulo analógico (4ch, 4-20mA)
 *   1    ALS-MPM-2F               Sensor de nivel sumergible
 *   1    Fuente 24VDC             Alimentación (mínimo 2A)
 *   1    Cable 2 hilos            Conexión sensor-módulo
 *   1    C0-00AC                  Base adaptadora (si se requiere)
 *   --   Cable USB                Para programación del PLC
 *
 * ============================================================================
 */
