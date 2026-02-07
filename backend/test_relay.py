"""
Script de diagnóstico para probar el relé del multímetro
Este script te ayudará a determinar si el problema es de código o de conexión
"""
import serial
import time
import sys

SERIAL_PORT = 'COM3'
BAUDRATE = 115200

def test_relay():
    print("=" * 60)
    print("🔧 DIAGNÓSTICO DEL RELÉ DE VOLTAJE")
    print("=" * 60)

    try:
        # Conectar al Arduino
        print(f"\n1. Conectando a {SERIAL_PORT}...")
        ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=2)
        time.sleep(2)  # Esperar inicialización del Arduino
        print("✓ Conectado exitosamente")

        # Leer mensaje de inicio del Arduino
        print("\n2. Leyendo mensaje de inicio del Arduino...")
        time.sleep(0.5)
        while ser.in_waiting:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"   Arduino dice: {line}")

        # Enviar comando START_VOLTAGE
        print("\n3. Enviando comando START_VOLTAGE...")
        ser.write(b"START_VOLTAGE\n")
        print("   Comando enviado: START_VOLTAGE")

        # Esperar respuesta
        print("\n4. Esperando respuesta del Arduino...")
        time.sleep(1)

        responses = []
        while ser.in_waiting:
            line = ser.readline().decode('utf-8').strip()
            if line:
                responses.append(line)
                print(f"   Arduino responde: {line}")

        if not responses:
            print("   ⚠️  No se recibió respuesta del Arduino")

        # Solicitar estado
        print("\n5. Solicitando estado del Arduino...")
        ser.write(b"GET_STATUS\n")
        time.sleep(0.5)

        while ser.in_waiting:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"   Estado: {line}")

        # Solicitar diagnóstico ADC
        print("\n6. Solicitando diagnóstico RAW ADC...")
        ser.write(b"GET_RAW_ADC\n")
        time.sleep(1)

        while ser.in_waiting:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"   Diagnóstico: {line}")

        # Esperar lecturas de voltaje
        print("\n7. Esperando lecturas de voltaje (5 segundos)...")
        print("   (Si el relé está activado, deberías ver lecturas aquí)")

        start_time = time.time()
        voltage_readings = []

        while time.time() - start_time < 5:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8').strip()
                if line and 'voltage' in line.lower():
                    voltage_readings.append(line)
                    print(f"   📊 {line}")

        # Enviar comando STOP_VOLTAGE
        print("\n8. Enviando comando STOP_VOLTAGE...")
        ser.write(b"STOP_VOLTAGE\n")
        time.sleep(0.5)

        while ser.in_waiting:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"   Arduino responde: {line}")

        # Resumen
        print("\n" + "=" * 60)
        print("📋 RESUMEN DEL DIAGNÓSTICO")
        print("=" * 60)

        if len(voltage_readings) > 0:
            print(f"✓ Se recibieron {len(voltage_readings)} lecturas de voltaje")
            print("✓ La comunicación serial está funcionando correctamente")
            print("\n🔍 VERIFICA FÍSICAMENTE:")
            print("   1. ¿Escuchaste un 'click' del relé al enviar START_VOLTAGE?")
            print("   2. ¿Hay un LED en el módulo de relé que se enciende?")
            print("   3. Mide con multímetro si hay continuidad en el relé")
            print("\n   Si NO escuchaste el click:")
            print("   → Problema de CONEXIÓN FÍSICA (revisa el pin 22 y el transistor)")
            print("\n   Si SÍ escuchaste el click:")
            print("   → El relé funciona correctamente ✓")
        else:
            print("✗ NO se recibieron lecturas de voltaje")
            print("\n🔍 POSIBLES CAUSAS:")
            print("   1. El Arduino no está recibiendo el comando")
            print("   2. El relé no se está activando")
            print("   3. Problema en el código del Arduino")
            print("\n   RECOMENDACIÓN:")
            print("   → Abre el Monitor Serial de Arduino IDE")
            print("   → Escribe manualmente: START_VOLTAGE")
            print("   → Verifica si ves el mensaje de activación")

        ser.close()
        print("\n✓ Diagnóstico completado")

    except serial.SerialException as e:
        print(f"\n✗ Error de conexión serial: {e}")
        print("\n🔍 VERIFICA:")
        print("   1. El Arduino está conectado al puerto COM3")
        print("   2. El Monitor Serial de Arduino IDE está CERRADO")
        print("   3. No hay otro programa usando el puerto serial")
    except KeyboardInterrupt:
        print("\n\n⚠️  Diagnóstico interrumpido por el usuario")
        if 'ser' in locals() and ser.is_open:
            ser.close()
    except Exception as e:
        print(f"\n✗ Error inesperado: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    test_relay()
