import pandas as pd
import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_squared_error
import time

def carregar_dados(arquivo_csv, n_amostras=None):
    """Carrega o CSV e converte tudo pra float, ignorando linhas inválidas."""
    dados = pd.read_csv(
        arquivo_csv,
        header=0,                # usa a primeira linha como cabeçalho se existir
        names=["x", "y"],        # define nomes se não existir cabeçalho
        on_bad_lines="skip",     # ignora linhas problemáticas
        dtype=str                # lê tudo como string primeiro
    )

    # converte para float (linhas inválidas viram NaN)
    dados["x"] = pd.to_numeric(dados["x"], errors="coerce")
    dados["y"] = pd.to_numeric(dados["y"], errors="coerce")

    # remove linhas com NaN (resultantes de erros de leitura)
    dados = dados.dropna(subset=["x", "y"])

    # converte pra numpy
    X = dados["x"].to_numpy(dtype=float)
    Y = dados["y"].to_numpy(dtype=float)

    # seleciona subconjunto se desejado
    if n_amostras is not None and n_amostras < len(X):
        idx = np.random.default_rng(42).choice(len(X), n_amostras, replace=False)
        X, Y = X[idx], Y[idx]
        # ordena por X (só pra manter visual consistente)
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

def testar_modelo(arquivo_csv, tamanhos=[1000, 10000, 100000, 1000000]):
    resultados = []

    for n in tamanhos:
        print(f"\n--- Testando com {n} pontos ---")
        X, Y = carregar_dados(arquivo_csv, n_amostras=n)

        inicio = time.time()
        A, B, modelo = regressao_linear(X, Y)
        tempo = time.time() - inicio

        # Previsão e cálculo do erro médio quadrático
        Y_pred = modelo.predict(X.reshape(-1, 1))
        mse = mean_squared_error(Y, Y_pred)

        print(f"A = {A:.6f}, B = {B:.6f}")
        print(f"MSE = {mse:.8f}")
        print(f"Tempo = {tempo:.6f} segundos")

        resultados.append((n, A, B, mse, tempo))

    return resultados

def main():
    nome_arquivo = "dados.csv"
    tamanhos = [1000, 10000, 100000, 1000000]  # escolha conforme seu hardware
    resultados = testar_modelo(nome_arquivo, tamanhos)

    print("\n=== RESUMO FINAL ===")
    print("N\tA\t\tB\t\tMSE\t\tTempo (s)")
    for n, A, B, mse, tempo in resultados:
        print(f"{n}\t{A:.6f}\t{B:.6f}\t{mse:.8f}\t{tempo:.4f}")

if __name__ == "__main__":
    main()
