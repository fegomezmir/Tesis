import json
from matplotlib import pyplot as plt
import numpy as np
from scipy import signal

nombre_archivo = 'mediciones_2021_10_22_23_ 4.6Mw 22 km al NO de El Tabo'
#nombre_archivo = 'mediciones_2021_11_3_0_5.7Mw 50 km al E de Los Andes'
#nombre_archivo = 'mediciones_ruido'

frecuencia_muestreo = 125


minuto_inicio = 1
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

def grafico_3ejes_spectrograma( accx, accy, accz, Fs):
    plt.subplot(3, 1, 1)
    f, t, Sxx = signal.spectrogram(np.array(accx), Fs)
    plt.pcolormesh(t, f, Sxx, shading='gouraud')

    plt.title("Eje X")

    plt.subplot(3, 1, 2)
    f, t, Sxx = signal.spectrogram(np.array(accy), Fs)
    plt.pcolormesh(t, f, Sxx, shading='gouraud')
    plt.ylabel("Frecuencia (Hz)")
    plt.title("Eje Y")

    plt.subplot(3, 1, 3)
    f, t, Sxx = signal.spectrogram(np.array(accz), Fs)
    plt.pcolormesh(t, f, Sxx, shading='gouraud')
    plt.xlabel("Tiempo (ms)")

    plt.title("Eje Z")
    plt.show()




f = open(nombre_archivo + '.json')


data = json.load(f)

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

datos = {}
mediciones = []

for id in data['2']:
    mediciones.append(data['2'][id])



mediciones_ordenadas = sorted(mediciones, key=lambda i: i['Millis'])
tiempo = []
for medicion in mediciones_ordenadas:
    #tiempo.append(medicion['Tiempo'])
    millis.append(medicion['Millis'])
    X.append(medicion['X'])
    Y.append(medicion['Y'])
    Z.append(medicion['Z'] )
    agno.append(medicion['Año'])
    mes.append(medicion['Mes'])
    dia.append(medicion['Dia'])
    hora.append(medicion['Hora'])
    minuto.append(medicion['Minuto'])


pos = 1
while pos < len(millis):
    if pos > len(millis):
        break
    if millis[pos] == millis[pos-1]:
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


delta = []
t_anterior = millis[0]
for t in millis:
    delta.append(t - t_anterior)
    t_anterior = t


t_inicio = millis[0]
tiempo2 = []
print(t_inicio)
for t in millis:
    tiempo2.append((t - t_inicio)/1000 + 28.742)




X_recortado, Y_recortado, Z_recortado, millis_recortado = recortar(minuto_inicio, minuto_fin, X, Y, Z, millis, minuto )


millis_zero = []
for t in millis_recortado:
    millis_zero.append(t - millis_recortado[0])


Xxf, Xyf = fft_mediciones(X_recortado, frecuencia_muestreo)
Yxf, Yyf = fft_mediciones(Y_recortado, frecuencia_muestreo)
Zxf, Zyf = fft_mediciones(Z_recortado, frecuencia_muestreo)

plt.subplot(3,1,1)
plt.plot(millis_zero, X_recortado)
plt.legend(["Eje X"])
plt.xlabel("Tiempo (ms)")
plt.ylabel("Aceleración (g)")
plt.subplot(3,1,2)
plt.plot(millis_zero, Y_recortado)
plt.legend(["Eje Y"])
plt.xlabel("Tiempo (ms)")
plt.ylabel("Aceleración (g)")
plt.subplot(3,1,3)
plt.plot(millis_zero, Z_recortado)
plt.legend(["Eje Z"])
plt.xlabel("Tiempo (ms)")
plt.ylabel("Aceleración (g)")
plt.show()

X_offset = []

mediaX = np.mean(X_recortado)
mediaY = np.mean(Y_recortado)
mediaZ = np.mean(Z_recortado)

X_offset =  X_recortado - mediaX
Y_offset = []

Y_offset= Y_recortado - mediaY
Z_offset = []

Z_offset= Z_recortado - mediaZ

plt.subplot(3,1,1)
plt.plot(millis_zero, X_offset)
plt.legend(["Eje X"])
plt.subplot(3,1,2)
plt.plot(millis_zero, Y_offset)
plt.legend(["Eje Y"])
plt.ylabel("Aceleración (g)")
plt.subplot(3,1,3)
plt.plot(millis_zero, Z_offset)
plt.legend(["Eje Z"])
plt.xlabel("Tiempo (ms)")
plt.show()





plt.subplot(3,1,1)
plt.semilogy(Xxf, Xyf)
plt.legend(["Eje X"])
plt.subplot(3,1,2)
plt.semilogy(Yxf, Yyf)
plt.legend(["Eje Y"])
plt.ylabel("Aceleración (g)")
plt.subplot(3,1,3)
plt.semilogy(Zxf, Zyf)
plt.legend(["Eje Z"])
plt.xlabel("Frecuencia (Hz)")
plt.show()


"""
PSDxy, PSDxx = plt.psd(X_recortado, NFFT=2048, Fs=125, scale_by_freq=False)
PSDyy, PSDyx = plt.psd(Y_recortado, NFFT=2048, Fs=125, scale_by_freq=False)
PSDzy, PSDzx = plt.psd(Z_recortado, NFFT=2048, Fs=125, scale_by_freq=False)
plt.legend(["Eje X", "Eje Y","Eje Z"])
plt.xlabel("Frecuencia (Hz)")
plt.ylabel("Densidad espectral de potencia (dB)")
plt.show()
"""


f, Pxx_den = signal.periodogram(X_recortado, fs = 125)
f, Pyy_den = signal.periodogram(Y_recortado, fs = 125)
f, Pzz_den = signal.periodogram(Z_recortado, fs = 125)

plt.semilogy(f, Pxx_den)
plt.semilogy(f, Pyy_den)
plt.semilogy(f, Pzz_den)
plt.ylim([1e-12, 1e-2])
plt.grid(True)
plt.legend(["Eje X", "Eje Y","Eje Z"])
plt.xlabel('Frecuencia (Hz)')
plt.ylabel('PSD (g^2/Hz)')
plt.show()




plt.plot(delta)
plt.show()



sqrtX = []
for i in X_offset:
    sqrtX.append(i**2)
sqrtY = []
for i in Y_offset:
    sqrtY.append(i**2)
sqrtZ = []
for i in Z_offset:
    sqrtZ.append(i**2)



print("Valor RMS eje X: "+ str(np.sqrt(np.mean(sqrtX))))
print("Valor RMS eje Y: "+ str(np.sqrt(np.mean(sqrtY))))
print("Valor RMS eje Z: "+ str(np.sqrt(np.mean(sqrtZ))))
print("Duración medición: " + str((millis[-1]-millis[0])/1000) + " segundos")


grafico_3ejes_spectrograma(X_recortado, Y_recortado, Z_recortado,frecuencia_muestreo)


f = open(nombre_archivo + " compilado.txt", "w")

f.write(" Medicion ADXL355 muestreando a 125Hz, escala en g  "+ "\n")
f.write(" Millis sensor, eje X, eje Y, eje Z  " + "\n")
f.write( "\n")
f.write( "\n")

for n in range(len(millis_zero)):
    f.write(str(millis_zero[n]) + " , " + str(X[n]) + " , " + str(Y[n]) + " , " + str(Z[n])+ "\n")
f.close()



