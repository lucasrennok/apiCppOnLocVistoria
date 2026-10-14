SET NAMES utf8mb4;

-- ===================== Schema =====================
-- `seq` garante a ordem de inserção nas listagens (os ids são strings).

CREATE TABLE users (
  id            VARCHAR(40)  NOT NULL PRIMARY KEY,
  seq           BIGINT       NOT NULL AUTO_INCREMENT UNIQUE,
  name          VARCHAR(255) NOT NULL,
  email         VARCHAR(255) NOT NULL UNIQUE,
  role          ENUM('Analista','Gestor') NOT NULL DEFAULT 'Analista',
  password_hash VARCHAR(255) NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE clients (
  id            VARCHAR(40)  NOT NULL PRIMARY KEY,
  seq           BIGINT       NOT NULL AUTO_INCREMENT UNIQUE,
  responsible   VARCHAR(255) NOT NULL,
  document      VARCHAR(14)  NOT NULL,
  document_type ENUM('CPF','CNPJ') NOT NULL,
  trade_name    VARCHAR(255) NOT NULL,
  location      VARCHAR(255) NOT NULL,
  created_at    DATETIME     NOT NULL,
  INDEX idx_clients_document (document)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE assets (
  id               VARCHAR(40)  NOT NULL PRIMARY KEY,
  seq              BIGINT       NOT NULL AUTO_INCREMENT UNIQUE,
  patrimony_number VARCHAR(50)  NOT NULL,
  serial_number    VARCHAR(100) NOT NULL,
  description      VARCHAR(500) NOT NULL,
  status           ENUM('disponivel','emprestado','avariado') NOT NULL DEFAULT 'disponivel',
  INDEX idx_assets_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE asset_sub_items (
  id               VARCHAR(40)  NOT NULL PRIMARY KEY,
  seq              BIGINT       NOT NULL AUTO_INCREMENT UNIQUE,
  asset_id         VARCHAR(40)  NOT NULL,
  name             VARCHAR(255) NOT NULL,
  patrimony_number VARCHAR(50)  NOT NULL,
  CONSTRAINT fk_sub_asset FOREIGN KEY (asset_id) REFERENCES assets(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE loans (
  id           VARCHAR(40)  NOT NULL PRIMARY KEY,
  seq          BIGINT       NOT NULL AUTO_INCREMENT UNIQUE,
  client_id    VARCHAR(40)  NOT NULL,
  organization VARCHAR(255) NOT NULL,
  loan_date    DATETIME     NOT NULL,
  details      TEXT         NULL,
  status       ENUM('sem avarias','com avarias') NOT NULL DEFAULT 'sem avarias',
  CONSTRAINT fk_loan_client FOREIGN KEY (client_id) REFERENCES clients(id),
  INDEX idx_loans_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE loan_items (
  id               VARCHAR(40) NOT NULL PRIMARY KEY,
  seq              BIGINT      NOT NULL AUTO_INCREMENT UNIQUE,
  loan_id          VARCHAR(40) NOT NULL,
  asset_id         VARCHAR(40) NOT NULL,
  patrimony_number VARCHAR(50) NOT NULL,
  returned         TINYINT(1)  NOT NULL DEFAULT 0,
  CONSTRAINT fk_li_loan  FOREIGN KEY (loan_id)  REFERENCES loans(id) ON DELETE CASCADE,
  CONSTRAINT fk_li_asset FOREIGN KEY (asset_id) REFERENCES assets(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE damages (
  id            VARCHAR(40) NOT NULL PRIMARY KEY,
  seq           BIGINT      NOT NULL AUTO_INCREMENT UNIQUE,
  loan_id       VARCHAR(40) NOT NULL,
  loan_item_id  VARCHAR(40) NOT NULL,
  description   TEXT        NOT NULL,
  photos        TEXT        NOT NULL,              -- JSON: string[]
  registered_by VARCHAR(40) NOT NULL,
  registered_at DATETIME    NOT NULL,
  CONSTRAINT fk_dmg_loan FOREIGN KEY (loan_id)      REFERENCES loans(id) ON DELETE CASCADE,
  CONSTRAINT fk_dmg_item FOREIGN KEY (loan_item_id) REFERENCES loan_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE inspections (
  id              VARCHAR(40) NOT NULL PRIMARY KEY,
  seq             BIGINT      NOT NULL AUTO_INCREMENT UNIQUE,
  client_id       VARCHAR(40) NOT NULL,
  loan_id         VARCHAR(40) NOT NULL,
  inspection_date DATETIME    NOT NULL,
  photos          TEXT        NOT NULL,            -- JSON: [{url, note?}]
  assessment      ENUM('avaria_por_mau_uso','sem_avaria') NOT NULL,
  approval        ENUM('pendente','aprovado','reprovado') NOT NULL,
  archived        TINYINT(1)  NOT NULL DEFAULT 0,
  CONSTRAINT fk_insp_client FOREIGN KEY (client_id) REFERENCES clients(id),
  CONSTRAINT fk_insp_loan   FOREIGN KEY (loan_id)   REFERENCES loans(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ===================== Seed (API-MOCK.md) =====================
-- password_hash fica NULL: a API define a senha dos usuários u1/u2 na subida
-- quando SEED_DEFAULT_PASSWORD está configurada (ver README).

INSERT INTO users (id, name, email, role) VALUES
  ('u1', 'Ana Souza',  'analista@onloc.com.br', 'Analista'),
  ('u2', 'Bruno Lima', 'gestor@onloc.com.br',   'Gestor');

INSERT INTO clients (id, responsible, document, document_type, trade_name, location, created_at) VALUES
  ('c1', 'Carlos Pereira', '12345678000199', 'CNPJ', 'Construtora Alfa Ltda', 'Campinas - SP', '2026-09-02 10:00:00'),
  ('c2', 'Mariana Alves',  '32987654321',    'CPF',  'Transportes Beta ME',   'Valinhos - SP', '2026-09-15 14:30:00');

INSERT INTO assets (id, patrimony_number, serial_number, description, status) VALUES
  ('a1', 'PAT-0001', 'SN-7788-01', 'Betoneira 400L',        'emprestado'),
  ('a2', 'PAT-0002', 'SN-7788-02', 'Martelete Rompedor',    'emprestado'),
  ('a3', 'PAT-0003', 'SN-7788-03', 'Andaime 2m (módulo)',   'disponivel');

INSERT INTO asset_sub_items (id, asset_id, name, patrimony_number) VALUES
  ('s1', 'a1', 'Cabo de força 5m',      'PAT-0001-A'),
  ('s2', 'a1', 'Carregador 110V',       'PAT-0001-B'),
  ('s3', 'a2', 'Ponteira 25mm',         'PAT-0002-A'),
  ('s4', 'a2', 'Maleta de transporte',  'PAT-0002-B');

INSERT INTO loans (id, client_id, organization, loan_date, details, status) VALUES
  ('l1', 'c1', 'Obra Residencial Jardim Europa', '2026-09-05 09:00:00', 'Locação de 30 dias',  'sem avarias'),
  ('l2', 'c2', 'Frota Beta — Manutenção',        '2026-09-20 15:00:00', 'Empréstimo avulso',   'com avarias');

INSERT INTO loan_items (id, loan_id, asset_id, patrimony_number, returned) VALUES
  ('li1', 'l1', 'a1', 'PAT-0001', 0),
  ('li2', 'l1', 'a2', 'PAT-0002', 0),
  ('li3', 'l2', 'a1', 'PAT-0001', 1);

INSERT INTO damages (id, loan_id, loan_item_id, description, photos, registered_by, registered_at) VALUES
  ('d1', 'l2', 'li3', 'Cabo de força rompido próximo ao plugue', '[]', 'u1', '2026-09-30 08:00:00');

INSERT INTO inspections (id, client_id, loan_id, inspection_date, photos, assessment, approval, archived) VALUES
  ('i1', 'c2', 'l2', '2026-09-30 08:00:00', '[]', 'avaria_por_mau_uso', 'aprovado', 1),
  ('i2', 'c1', 'l1', '2026-10-01 10:00:00', '[]', 'sem_avaria',         'pendente',  0);
