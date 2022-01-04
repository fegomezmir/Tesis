import json
from matplotlib import pyplot as plt
import numpy as np
from scipy import signal
from scipy.fft import fftshift


#nombre_archivo = 'mediciones_2021_11_3_0_5.7Mw 50 km al E de Los Andes'
nombre_archivo = "mediciones_2021_11_10_2_ 4.6Mw 6 km al SE de Quillota multiples sensores"
#nombre_archivo = 'coordinacion 2'

frecuencia_muestreo = 125


minuto_inicio = 0
minuto_fin = 60

def fft_mediciones(medicion, frecuencia_m):
    t = np.arange(len(medicion) // 2)
    sp = np.fft.fft(medicion) / len(medicion)
    sp = sp[range(int(len(medicion) / 2))]
    freq = t / (len(medicion) / frecuencia_m)
    return freq, abs(sp)



def recortar(minuto_inicio, minuto_fin, x, y, z, millis, minutos):
    pos_inicio = 0
    pos_fin = 0
    for m in minutos:
        if m >= minuto_inicio:
            break
        pos_inicio += 1

    for m in minutos:
        if m >= minuto_fin:
            break
        pos_fin +=1

    x_recortado = x[pos_inicio:pos_fin]
    y_recortado = y[pos_inicio:pos_fin]
    z_recortado = z[pos_inicio:pos_fin]
    millis_recortado = millis[pos_inicio:pos_fin]
    return x_recortado, y_recortado, z_recortado, millis_recortado

def grafico_3ejes(dict_nodos, eje_x1, eje_y1, eje_x2, eje_y2, eje_x3, eje_y3):
    plt.subplot(3, 1, 1)
    nodos = []
    for i in range(0, 10):
        try:
            nodos.append("nodo_" + str(i))
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x1], dict_nodos["nodo_" + str(i)][eje_y1])
            plt.xlabel(eje_x1)
            plt.ylabel("Aceleración eje" +eje_y1 + " (g)")
            plt.legend(nodos)
        except:
            continue
    plt.subplot(3, 1, 2)
    for i in range(0, 10):
        try:
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x2], dict_nodos["nodo_" + str(i)][eje_y2])
            plt.xlabel(eje_x2)
            plt.ylabel("Aceleración eje" +eje_y2 + " (g)")
            plt.legend(nodos)
        except:
            continue
    plt.subplot(3, 1, 3)
    for i in range(0, 10):
        try:
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x3], dict_nodos["nodo_" + str(i)][eje_y3])
            plt.xlabel(eje_x3)
            plt.ylabel("Aceleración eje" +eje_y3 + " (g)")
            plt.legend(nodos)
        except:
            continue
    plt.show()

def grafico_3ejes_extra(dict_nodos, eje_x1, eje_y1, eje_x2, eje_y2, eje_x3, eje_y3):
    #plt.subplot(3, 1, 1)
    nodos = []
    for i in range(0, 10):
        try:
            nodos.append("nodo_" + str(i))
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x1], dict_nodos["nodo_" + str(i)][eje_y1])
            plt.xlabel(eje_x1)
            plt.xlabel("Tiempo (ms)")
            plt.ylabel("Aceleración eje X (g)")
            plt.legend(nodos)
        except:
            continue
    plt.show()
    #plt.subplot(3, 1, 2)
    for i in range(0, 10):
        try:
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x2], dict_nodos["nodo_" + str(i)][eje_y2])
            plt.xlabel(eje_x2)
            plt.xlabel("Tiempo (ms)")
            plt.ylabel("Aceleración eje Y (g)")
            plt.legend(nodos)
        except:
            continue
    plt.show()
    #plt.subplot(3, 1, 3)
    for i in range(0, 10):
        try:
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x3], dict_nodos["nodo_" + str(i)][eje_y3])
            plt.xlabel("Tiempo (ms)")
            plt.ylabel("Aceleración eje Z (g)")
            plt.legend(nodos)
        except:
            continue
    plt.show()

def grafico_3ejes_fft(dict_nodos, eje_x1, eje_y1, eje_x2, eje_y2, eje_x3, eje_y3):
    plt.subplot(3, 1, 1)
    nodos = []
    for i in range(0, 10):
        try:
            nodos.append("nodo_" + str(i))
            plt.semilogy(dict_nodos["nodo_" + str(i)][eje_x1], dict_nodos["nodo_" + str(i)][eje_y1])
            plt.ylabel("Eje X")
            plt.legend(nodos)
        except:
            continue
    plt.subplot(3, 1, 2)
    for i in range(0, 10):
        try:
            plt.semilogy(dict_nodos["nodo_" + str(i)][eje_x2], dict_nodos["nodo_" + str(i)][eje_y2])
            plt.ylabel("Eje Y")
            plt.legend(nodos)
        except:
            continue
    plt.subplot(3, 1, 3)
    for i in range(0, 10):
        try:
            plt.semilogy(dict_nodos["nodo_" + str(i)][eje_x3], dict_nodos["nodo_" + str(i)][eje_y3])
            plt.ylabel("Eje Z")
            plt.xlabel("Frecuencia (Hz)")
            plt.legend(nodos)
        except:
            continue
    plt.show()



def grafico_3ejes_spectrograma(dict_nodos,nodo, acc1, Fs):

    nodos = []
    nodos.append("nodo_" + str(nodo))
    f, t, Sxx = signal.spectrogram(np.array(dict_nodos["nodo_" + str(nodo)][acc1]), Fs)
    plt.pcolormesh(t, f, Sxx, shading='gouraud')
    plt.xlabel("Tiempo")
    plt.ylabel("Frecuencia")
    plt.show()



def grafico_PSD_1_canal(dict_nodos, eje_recortado, title):
    nodos = []
    for i in range(0, 10):
        try:
            nodos.append("nodo_" + str(i))

            f, Pxx_den = signal.periodogram(np.array(dict_nodos["nodo_" + str(i)][eje_recortado]), fs=125)
            plt.semilogy(f, Pxx_den)
            plt.ylim([1e-10, 1e-1])
            plt.grid(True)
            plt.legend(nodos)
            plt.xlabel('Frecuencia (Hz)')
            plt.ylabel('Densidad espectral de potencia (g^2/Hz)')
            plt.title(title)


        except:
            continue
    plt.show()




f = open(nombre_archivo + '.json')


data = json.load(f)

dict_nodos = {}

datos = {}
mediciones = []

for id_nodo in range(0, 10):

    tiempo = []
    X = []
    Y = []
    Z = []
    agno = []
    mes = []
    dia = []
    hora = []
    minuto = []
    segundo = []
    millis = []
    ID = []
    X_recortado = []
    Y_recortado = []
    Z_recortado = []
    millis_recortado = []

    datos = {}
    mediciones = []

    try:
        nodo = data[id_nodo]
        if nodo == None:
            continue
    except:
        continue

    for id in nodo:
        mediciones.append(nodo[id])

    mediciones_ordenadas = sorted(mediciones, key=lambda i: i['Millis'])
    tiempo = []
    for medicion in mediciones_ordenadas:
        # tiempo.append(medicion['Tiempo'])
        millis.append(medicion['Millis'])
        X.append(medicion['X'])
        Y.append(medicion['Y'])
        Z.append(medicion['Z'])
        agno.append(medicion['Año'])
        mes.append(medicion['Mes'])
        dia.append(medicion['Dia'])
        hora.append(medicion['Hora'])
        minuto.append(medicion['Minuto'])

    pos = 1
    while pos < len(millis):
        if pos > len(millis):
            break
        if millis[pos] == millis[pos - 1]:
            del millis[pos]
            del X[pos]
            del Y[pos]
            del Z[pos]
            del agno[pos]
            del mes[pos]
            del dia[pos]
            del hora[pos]
            del minuto[pos]
        else:
            pos += 1

    X_recortado, Y_recortado, Z_recortado, millis_recortado = recortar(minuto_inicio, minuto_fin, X, Y, Z, millis, minuto)

    t_inicio = millis[0]
    tiempo2 = []
    print(t_inicio)
    for t in millis:
        tiempo2.append((t - t_inicio))

    delta = []
    t_anterior = millis[0]
    for t in millis:
        delta.append(t - t_anterior)
        t_anterior = t

    millis_zero = []
    for t in millis_recortado:
        millis_zero.append(t - millis_recortado[0])

    Xxf, Xyf = fft_mediciones(X_recortado, frecuencia_muestreo)
    Yxf, Yyf = fft_mediciones(Y_recortado, frecuencia_muestreo)
    Zxf, Zyf = fft_mediciones(Z_recortado, frecuencia_muestreo)

    X_offset = []
    Y_offset = []
    Z_offset = []

    mediaX = np.mean(X_recortado)
    mediaY = np.mean(Y_recortado)
    mediaZ = np.mean(Z_recortado)

    X_offset = X_recortado - mediaX
    Y_offset = Y_recortado - mediaY
    Z_offset = Z_recortado - mediaZ

    datos["X"] = X
    datos["Y"] = Y
    datos["Z"] = Z
    datos["millis"] = millis
    datos["tiempo2"] = tiempo2
    datos["Xxf"] = Xxf
    datos["Xyf"] = Xyf
    datos["Yxf"] = Yxf
    datos["Yyf"] = Yyf
    datos["Zxf"] = Zxf
    datos["Zyf"] = Zyf
    datos["delta"] = delta
    datos["Año"] = agno
    datos["Mes"] = mes
    datos["Dia"] = dia
    datos["Hora"] = hora
    datos["X_recortado"] = X_recortado
    datos["Y_recortado"] = Y_recortado
    datos["Z_recortado"] = Z_recortado
    datos["millis_recortado"] = millis_recortado
    datos["X_offset"] = X_offset
    datos["Y_offset"] = Y_offset
    datos["Z_offset"] = Z_offset


    dict_nodos["nodo_" + str(id_nodo)] = datos


grafico_3ejes(dict_nodos, "millis", "X", "millis", "Y", "millis", "Z")
grafico_3ejes_extra(dict_nodos, "millis", "X_offset", "millis", "Y_offset", "millis", "Z_offset")
grafico_3ejes_fft(dict_nodos, "Xxf", "Xyf", "Yxf", "Yyf", "Zxf", "Zyf")
grafico_3ejes(dict_nodos, "tiempo2", "delta", "tiempo2", "millis", "", "")

grafico_PSD_1_canal(dict_nodos, "X_recortado", "Eje X")
grafico_PSD_1_canal(dict_nodos, "Y_recortado", "Eje Y")
grafico_PSD_1_canal(dict_nodos, "Z_recortado", "Eje Z")


grafico_3ejes_spectrograma(dict_nodos, 2, "X_recortado",frecuencia_muestreo)
grafico_3ejes_spectrograma(dict_nodos, 2, "Y_recortado",frecuencia_muestreo)
grafico_3ejes_spectrograma(dict_nodos, 2, "Z_recortado",frecuencia_muestreo)







max_length = 0
for i in range(0,10):
    try:
        if len(dict_nodos["nodo_" + str(i)]["tiempo2"]) > max_length:
            max_length = len(dict_nodos["nodo_" + str(i)]["tiempo2"])
            print(max_length)
    except:
        continue


f = open(nombre_archivo + " compilado.txt", "w")

f.write(" Medicion ADXL355 muestreando a 125Hz, escala en g  "+ "\n")
f.write(" Millis sensor 0, eje X sensor 0, eje Y sensor 0, eje Z sensor 0, "
        "Millis sensor 1, eje X sensor 1, eje Y sensor 1, eje Z sensor 1, "
        "Millis sensor 2, eje X sensor 2, eje Y sensor 2, eje Z sensor 2, "
        "Millis sensor 3, eje X sensor 3, eje Y sensor 3, eje Z sensor 3, "
        "Millis sensor 4, eje X sensor 4, eje Y sensor 4, eje Z sensor 4, "
        "Millis sensor 5, eje X sensor 5, eje Y sensor 5, eje Z sensor 5, "
        "Millis sensor 6, eje X sensor 6, eje Y sensor 6, eje Z sensor 6, "
        "Millis sensor 7, eje X sensor 7, eje Y sensor 7, eje Z sensor 7, "
        "Millis sensor 8, eje X sensor 8, eje Y sensor 8, eje Z sensor 8, "
        "Millis sensor 9, eje X sensor 9, eje Y sensor 9, eje Z sensor 9, "
         + "\n")
f.write( "\n")
f.write( "\n")


for i in range(max_length):
    String = ""
    for id in range(0,10):
        try:
            String = String + str(dict_nodos["nodo_" + str(id)]["tiempo2"][i])+ ", " + \
                 str(dict_nodos["nodo_" + str(id)]["X_recortado"][i]) + ", " + \
                 str(dict_nodos["nodo_" + str(id)]["Y_recortado"][i]) + ", " + \
                 str(dict_nodos["nodo_" + str(id)]["Z_recortado"][i]) +", "
        except:
            String = String + "0, 0, 0, 0, "
    String += "\n"
    f.write(String)
f.close()