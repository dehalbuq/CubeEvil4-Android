# CloudLab v4 — Fase 5: ponte sintética SVC/TLS

- Fases 1–4 mantidas intactas.
- Fase 5: números SVC públicos 0x1F, 0x21, 0x16, IPC de 256 bytes.
- O host diferencia o retorno de SVC (X0) do código de resposta SFCO.
- Endereços do convidado são offsets limitados a 4096 bytes.
- Unknown SVCs / descritores / serviços reais: NOT_SUPPORTED.
- Sintético e sem execução de RE4 ou instruções SVC originais.
- Testes locais CTest, ASAN, UBSAN e stress: conferidos ao construir v4.
- ARM64 compilado; runner GitHub Actions ainda sem execução na branch.
- Nuvem usa Mesa llvmpipe; não substitui driver Mali-G31 real.
