import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from matplotlib import pyplot as plt
import numpy as np
#from scipy.fft import fft, fftfreq
from multiprocessing.pool import ThreadPool as Pool
import json

cred = credentials.Certificate("tesis-4bc6a-firebase-adminsdk-504ot-315e938ea4.json")  # archivo con la autorizacion
firebase_admin.initialize_app(cred, {'databaseURL': 'https://tesis-4bc6a-default-rtdb.firebaseio.com'})

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

mediciones = []




frecuencia_muestreo = 125

def fft_mediciones(medicion):

    t = np.arange(len(medicion) // 2)
    sp = np.fft.fft(medicion) / len(medicion)
    sp = sp[range(int(len(medicion) / 2))]
    freq = t / (len(medicion) / frecuencia_muestreo)
    return freq, abs(sp)




def string_to_list(string):
    lista = string.split(',')
    del lista[0]
    return lista




pool_size = 50  # your "parallelness"

def worker_delete(fecha, id):
    try:
        print("borrando id: " + str(id) + " de " + fecha)
        db.reference("mediciones/" +fecha + "/" + id).delete()
    except Exception as ex:
        print(ex)
        print('error borrando')



def worker(medicion, id, id_nodo):
    try:
        clasificar(medicion, id_nodo)
        print("borrando id: " + str(id) + " de pool")
        db.reference('/mediciones/pool/' + str(id_nodo)+ '/' + id).delete()
    except Exception as ex:
        print(ex)
        print('error with medicion')


def clasificar(medicion, id_nodo):
    X = string_to_list(medicion['X'])
    Y = string_to_list(medicion['Y'])
    Z = string_to_list(medicion['Z'])
    millis = string_to_list(medicion['Millis'])
    segundo = string_to_list(medicion['Segundo'])
    minuto = string_to_list(medicion['Minuto'])
    hora = string_to_list(medicion['Hora'])
    dia = string_to_list(medicion['Dia'])
    mes = string_to_list(medicion['Mes'])
    agno = string_to_list(medicion['Agno'])

    pos_valor = 0
    while pos_valor < len(X):
        print(
            "Instancia " + str(pos_valor) + " de " + str(len(X)) )

        instancia_agno = int(agno[pos_valor])
        instancia_mes = int(mes[pos_valor])
        instancia_dia = int(dia[pos_valor])
        instancia_hora = int(hora[pos_valor])
        instancia_minuto = int(minuto[pos_valor])
        instancia_segundo = int(segundo[pos_valor])
        instancia_millis = float(millis[pos_valor])
        instancia_X = float(X[pos_valor])
        instancia_Y = float(Y[pos_valor])
        instancia_Z = float(Z[pos_valor])
        #instancia_tiempo = suma_tiempos(instancia_minuto, instancia_segundo, instancia_millis)

        ref = db.reference('mediciones/' + str(instancia_agno) + "/" + str(instancia_mes) + "/" + str(instancia_dia) + "/" + str(instancia_hora) + "/" + str(id_nodo))
        ref.push(
            {
                "Año": instancia_agno,
                "Mes": instancia_mes,
                "Dia": instancia_dia,
                "Hora": instancia_hora,
                "Minuto": instancia_minuto,
                "Segundo": instancia_segundo,
                "Millis":instancia_millis,
                "X": instancia_X,
                "Y": instancia_Y,
                "Z": instancia_Z,
                #"Tiempo": instancia_tiempo

            }
        )
        pos_valor += 1


def suma_tiempos( minuto, segundo, millis):
    suma = minuto * 60*1000.0 + segundo*1000.0+ millis
    return suma



def ordenar():
    print("Ordenando....")
    for id_nodo in range(10):
        ref = db.reference('/mediciones/pool/' + str(id_nodo))
        nodo = ref.get()
        print(nodo)
        if nodo == None:
            continue


        pool = Pool(pool_size)

        for id in nodo:
            print(id)
            pool.apply_async(worker, (nodo[id], id, id_nodo,))

        pool.close()
        pool.join()

    print("Orden finalizado")



def grafico_3ejes(dict_nodos, eje_x1, eje_y1, eje_x2, eje_y2, eje_x3, eje_y3):
    plt.subplot(3, 1, 1)
    nodos = []
    for i in range(0, 10):
        try:
            nodos.append("nodo_" + str(i))
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x1], dict_nodos["nodo_" + str(i)][eje_y1])
            plt.xlabel(eje_x1)
            plt.ylabel(eje_y1)
            plt.legend(nodos)
        except:
            continue
    plt.subplot(3, 1, 2)
    for i in range(0, 10):
        try:
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x2], dict_nodos["nodo_" + str(i)][eje_y2])
            plt.xlabel(eje_x2)
            plt.ylabel(eje_y2)
            plt.legend(nodos)
        except:
            continue
    plt.subplot(3, 1, 3)
    for i in range(0, 10):
        try:
            plt.plot(dict_nodos["nodo_" + str(i)][eje_x3], dict_nodos["nodo_" + str(i)][eje_y3])
            plt.xlabel(eje_x3)
            plt.ylabel(eje_y3)
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
            plt.xlabel(eje_x1)
            plt.ylabel(eje_y1)
            plt.legend(nodos)
        except:
            continue
    plt.subplot(3, 1, 2)
    for i in range(0, 10):
        try:
            plt.semilogy(dict_nodos["nodo_" + str(i)][eje_x2], dict_nodos["nodo_" + str(i)][eje_y2])
            plt.xlabel(eje_x2)
            plt.ylabel(eje_y2)
            plt.legend(nodos)
        except:
            continue
    plt.subplot(3, 1, 3)
    for i in range(0, 10):
        try:
            plt.semilogy(dict_nodos["nodo_" + str(i)][eje_x3], dict_nodos["nodo_" + str(i)][eje_y3])
            plt.xlabel(eje_x3)
            plt.ylabel(eje_y3)
            plt.legend(nodos)
        except:
            continue
    plt.show()



opciones = input("ordenar, leer, borrar todo? (o/l/b): ")

if opciones == "o":
    ordenar()

if opciones == "l":
    fecha_base = "2021/12/23/10/"

    dict_nodos = {}

    for id_nodo in range(0,10):

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
        fecha = fecha_base + str(id_nodo)
        ref = db.reference('/mediciones/' + fecha)
        nodo = ref.get()

        if nodo == None:
            continue

        for id in nodo:
            mediciones.append(nodo[id])


        mediciones_ordenadas = sorted(mediciones, key=lambda i: i['Millis'])
        tiempo = []
        for medicion in mediciones_ordenadas:
            #tiempo.append(medicion['Tiempo'])
            millis.append(medicion['Millis'])
            X.append(medicion['X'])
            Y.append(medicion['Y'])
            Z.append(medicion['Z'])
            agno.append(medicion['Año'])
            mes.append(medicion['Mes'])
            dia.append(medicion['Dia'])
            hora.append(medicion['Hora'])



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

        Xxf, Xyf = fft_mediciones(X)
        Yxf, Yyf = fft_mediciones(Y)
        Zxf, Zyf = fft_mediciones(Z)


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

        dict_nodos["nodo_"+ str(id_nodo)] = datos


    grafico_3ejes(dict_nodos, "millis", "X", "millis", "Y", "millis", "Z")
    grafico_3ejes(dict_nodos, "tiempo2", "X", "tiempo2", "Y", "tiempo2", "Z")
    grafico_3ejes_fft(dict_nodos, "Xxf", "Xyf", "Yxf", "Yyf", "Zxf", "Zyf")
    grafico_3ejes(dict_nodos, "tiempo2", "delta", "tiempo2", "millis", "", "")

    guardar = input("guardar datos del dia? (S/N): ")
    if guardar == "S":
        ref = db.reference('/mediciones/' + fecha_base)
        data = ref.get()
        try:
            fecha_guardar = fecha_base.replace("/", "_")
            with open('mediciones_' + fecha_guardar + '.json', 'w') as fp:
                json.dump(data, fp)

        except Exception as ex:
            print(ex)
            print('error with medicion')

        """"for nodo in range(10):
            try:
                f = open( 'medicion_nodo'+ str(nodo)+ ", " + str(dict_nodos["nodo_" + str(nodo)]["Año"][0]) + "/"+ str(dict_nodos["nodo_" + str(nodo)]["Mes"][0]) +"/"+ str(dict_nodos["nodo_" + str(nodo)]["Dia"][0]) + "/"+ str(dict_nodos["nodo_" + str(nodo)]["Hora"][0]) + ".csv"  , 'w')

                writer = csv.writer(f)
                for pos_linea in range(len(str(dict_nodos["nodo_" + str(nodo)]["X"]))):
                    writer.writerow(str(dict_nodos["nodo_" + str(nodo)]["millis"]) +","+str(dict_nodos["nodo_" + str(nodo)]["X"])+","+str(dict_nodos["nodo_" + str(nodo)]["Y"])+","+str(dict_nodos["nodo_" + str(nodo)]["Z"]))
                f.close()
            except Exception as ex:
                print(ex)
                print('error with medicion')"""



    borrar = input("Borrar datos del dia? (S/N): ")

    if borrar == "S":
        try:

            ref = db.reference('/mediciones/' + fecha_base)
            ref.delete()

        except:
            print("demasiados datos para borrar")
            for id_nodo in range(10):
                fecha = fecha_base + str(id_nodo)
                print("Borrando datos...")
                ref = db.reference('/mediciones/' + fecha)
                nodo = ref.get()
                if nodo == None:
                    continue
                pool = Pool(pool_size)
                for id in nodo:

                    print(id)
                    pool.apply_async(worker_delete, (fecha, id,))

                pool.close()
                pool.join()
        print("Datos borrados")


if opciones == "b":
    borrar_todo = input("Borrar todos los datos? (S/N)")
    if borrar_todo == "S":
        print("Borrando datos...")
        db.reference('/mediciones').delete()
        print("Datos borrados")
    elif borrar_todo == "N":
        borrar_todo = input("Borrar datos de pool? (S/N)")
    elif borrar_todo == "S":
        db.reference('/mediciones/pool').delete()
    elif borrar_todo == "N":
        borrar_todo = input("Borrar datos de un día? (S/N)")
    elif borrar_todo == "S":
        borrar_fecha = input("Que dia? (Numero)")
        for i in range(24):
            db.reference('/mediciones/2021/11/' + borrar_fecha + "/" + str(i)).delete()
        print("Datos del " + borrar_fecha + " borrados")
    elif borrar_todo == "N":
        borrar_todo = input("Borrar datos de un año? (S/N)")
    elif borrar_todo == "S":
        borrar_fecha = input("Que año? (Numero)")
        for i in range(24):
            db.reference('/mediciones/' + borrar_fecha + "/" + str(i)).delete()
        print("Datos del " + borrar_fecha + " borrados")
    elif borrar_todo == "N":
        borrar_todo = input("Borrar datos de una hora? (S/N)")
    elif borrar_todo == "S":
        borrar_fecha = input("Que hora? (Numero)")
        db.reference('/mediciones/2021/11/' + borrar_fecha + "/" + str(i)).delete()
        print("Datos del " + borrar_fecha + " borrados")