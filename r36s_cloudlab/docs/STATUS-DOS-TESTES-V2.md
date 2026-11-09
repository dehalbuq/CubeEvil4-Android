# dArkOSRE CloudLab v2 — relatorio de validacao

## Observacao de origem

Profile recebido: `darkosre-cloudprofile-20250904-075432.zip`.
Manifesto SHA-256: 68/68 hashes validos. O ZIP bruto nao e necessario
para os testes de CI e nao acompanha este pacote. O resumo minimizado
esta em `R36S-DARKOSRE-REAL-PROFILE.json`.

## Resultado efetivamente executado neste ambiente de desenvolvimento

- Verificacao de integridade do perfil real: PASS.
- Regressao do verificador de arquivos ZIP (6 casos): PASS.
- Suite RE4NX Phase1–3 no x86_64 (6 testes CTest): PASS.
- Regressoes negativas e positivas de NSO0/LZ4/SHA256: 9 casos PASS.
- Regressoes de IPC/handles Horizon sintetico: 3 casos PASS.
- Teste de paginas de memoria 64, 256 e 512 MiB com escrita/leitura: PASS.
- Threads POSIX, mmap e transicao RW/RX de memoria: PASS.
- Compilacao cruzada de executaveis sinteticos ARM64 Phase2 e Phase3: PASS.
- Verificacao dos ELF AArch64 e ausencia de interpretador e dependencias
  dinamicas: PASS.
- Execucao de binarios ARM64 na nuvem Linux: PENDENTE (workflow preparado).
- Mesa EGL/GLES3 no computador atual: NAO EXECUTADO (faltam headers).
- Container ARM64 Debian 13 com cgroup de 895 MiB: NAO EXECUTADO aqui
  (Docker nao esta disponivel nesta maquina).
- GPU Mali-G31 real e FPS: NAO EXECUTADO, exigira o console.

O CI usa Ubuntu 24.04 ARM64 para testar codigo nativo e Debian 13 ARM64
em container com `--memory=895m --memory-swap=895m`, sem imitar o
kernel 4.4.189 ou a GPU Mali-G31. GLES3 e exercitado com Mesa llvmpipe.

**Importante:** os executaveis comerciais da Nintendo Switch **nao
sao executados** pelo RE4NX desta fase. A camada Horizon e os drivers
NVN ainda nao foram implementados. Sucesso dos testes sinteticos nao
implica compatibilidade de Resident Evil 4 de Switch.
