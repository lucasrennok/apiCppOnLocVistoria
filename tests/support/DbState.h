#pragma once

// Estado global compartilhado pelos testes de integração: indica se o banco
// de dados estava disponível quando o processo de teste iniciou.
namespace dbtest {

bool available();
void setAvailable(bool value);

}  // namespace dbtest
