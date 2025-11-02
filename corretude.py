import pandas as pd
import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_squared_error
import time

def carregar_dados(arquivo_csv, n_amostras=None):
    """Carrega o CSV e converte tudo pra float, ignorando linhas inválidas."""
    dados = pd.read_csv(
        arquivo_csv,
        header=0,
        names=["x", "y"],
        on_bad_lines="skip",
        dtype=str
    )

    dados["x"] = pd.to_numeric(dados["x"], errors="coerce")
    dados["y"] = pd.to_numeric(dados["y"], errors="coerce")
    dados = dados.dropna(subset=["x", "y"])

    X = dados["x"].to_numpy(dtype=float)
    Y = dados["y"].to_numpy(dtype=float)

    if n_amostras is not None and n_amostras < len(X):
        idx = np.random.default_rng(42).choice(len(X), n_amostras, replace=False)
        X, Y = X[idx], Y[idx]
        order = np.argsort(X)
        X, Y = X[order], Y[order]

    return X, Y

def regressao_linear(X, Y):
    modelo = LinearRegression()
    X = X.reshape(-1, 1)
    modelo.fit(X, Y)
    A = modelo.intercept_
    B = modelo.coef_[0]
    return A, B, modelo

def testar_modelo(arquivo_csv, tamanhos=[1000, 10000, 100000, 1000000], n_iter=3):
    resultados = []

    for n in tamanhos:
        print(f"\n--- Testando com {n} pontos ---")
        A_list, B_list, mse_list, tempo_list = [], [], [], []

        for i in range(n_iter):
            print(f"Iteração {i+1}...")
            X, Y = carregar_dados(arquivo_csv, n_amostras=n)

            inicio = time.time()
            A, B, modelo = regressao_linear(X, Y)

            Y_pred = modelo.predict(X.reshape(-1, 1))
            mse = mean_squared_error(Y, Y_pred)

            tempo = time.time() - inicio
            print(f"  A = {A:.6f}, B = {B:.6f}, MSE = {mse:.8f}, Tempo = {tempo:.6f}s")

            A_list.append(A)
            B_list.append(B)
            mse_list.append(mse)
            tempo_list.append(tempo)

        # Calcula média das métricas
        A_mean = np.mean(A_list)
        B_mean = np.mean(B_list)
        mse_mean = np.mean(mse_list)
        tempo_mean = np.mean(tempo_list)

        print(f"=== Médias para {n} pontos ===")
        print(f"A = {A_mean:.6f}, B = {B_mean:.6f}, MSE = {mse_mean:.8f}, Tempo = {tempo_mean:.6f}s")

        resultados.append((n, A_mean, B_mean, mse_mean, tempo_mean))

    return resultados

def main():
    nome_arquivo = "dados.csv"
    tamanhos = [10000, 100000, 1000000, 10000000]
    resultados = testar_modelo(nome_arquivo, tamanhos, n_iter=10)

    print("\n=== RESUMO FINAL ===")
    print("N\tA\t\tB\t\tMSE\t\tTempo (s)")
    for n, A, B, mse, tempo in resultados:
        print(f"{n}\t{A:.6f}\t{B:.6f}\t{mse:.8f}\t{tempo:.4f}")

if __name__ == "__main__":
    main()
