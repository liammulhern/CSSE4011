<h1 align="center">
  <img src="https://github.com/liammulhern/CSSE4011/blob/task_supplychain_dashboard/docs/images/PathLedger.png" alt="PathLedger">
</h1>

[![License](https://img.shields.io/badge/License-GPL%203.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)


# CSSE4011 Product Lifecycle Blockchain Passport

**Project and Scenario Description**  
An end-to-end IoT + blockchain solution that creates a tamper-proof “Digital Product Passport” (DPP) for physical goods. Each manufactured product carries a unique QR code that encodes a link to the user dashboard with JWT for authentication and hash to it's block chain model. Embedded sensor nodes (e.g. thingy52) record environmental data (temperature, humidity, location, custody transfers, impact) throughout the supply chain, hashing and anchoring snapshots on the IOTA blockchain. Clients placing a product order specify the requirements that the products must be subject to while in distribution. They are then alerted if the product exceeds these specifications. Sensor nodes also offer an NFC interface that can be scanned and link to the user dashboard identifying alterts. Stakeholders—from suppliers to consumers—scan the QR code to instantly verify provenance, integrity, and handling history, improving trust, compliance, and recall responsiveness.

---

## Deliverables & Key Performance Indicators

| Deliverable                                               | KPI / Success Metric                                         |
|-----------------------------------------------------------|--------------------------------------------------------------|
| **1. IoT Sensor Prototype**<br>MCU firmware for environmental sensing and tagging.  | ≥ 99 % packet delivery rate over LoRaWAN/NB-IoT in tests.     |
| **2. Blockchain Smart Contract**<br>Permissioned chain contract to store event hashes. | Record ≥ 1 000 on-chain events without error.                 |
| **3. Off-chain Data Store Integration**<br>Raw sensor logs in Azure Cosmos DB or IPFS. | Average write latency < 200 ms; ≥ 99.9 % data durability.     |
| **4. Web/Mobile Dashboard**<br>DPP lookup, analytics, alerting UI.                   | Page load ≤ 300 ms; user task success rate ≥ 95 % in usability tests. |
| **5. ABAC Permission Engine**<br>Attribute-based access control for all stakeholders. | 100 % enforcement of role policies in penetration tests.     |
| **6. End-to-End Traceability Demo**<br>Consumer-facing app scan → full history.     | Time to display full provenance ≤ 5 s; zero incorrect records.|

---

## System Overview

### Hardware Architecture

```mermaid
flowchart LR
    A[Manufactured Product<br/>(QR/NFC Tag)] -->|Scanning| B[Embedded Sensor Node<br/>(MCU+Temp/Humidity/GPS)]
    B -->|LoRaWAN/NB-IoT| C[Edge Gateway<br/>(Raspberry Pi)]
    C -->|MQTT → Azure IoT Hub| D[Azure IoT Hub]
    D -->|Event Processing| E[Azure Function → Cosmos DB]
    D -->|Blockchain API| F[Blockchain Network]
    F --> G[Smart Contract Ledger]
    E --> H[Web/Mobile Dashboard]
    G --> H
```

### Software Implementation Flow

```mermaid
flowchart TD
    subgraph Mote (Sensor Node)
      M1[Boot, Read Tag UID]
      M2[Sample Sensors<br/>(temp, humidity, GPS)]
      M3[Hash Payload<br/>(SHA-256)]
      M4[Transmit via LoRa/MQTT]
      M1 --> M2 --> M3 --> M4
    end

    subgraph Edge & Cloud
      E1[Edge Gateway<br/>(MQTT Broker)]
      E2[Batch & Sign TXs]
      E3[Azure IoT Hub → Event Grid]
      E4[Azure Function]
      E5[Store Raw Data → Cosmos DB/IPFS]
      E6[Submit Hash to Blockchain<br/>(Web3/Fabric SDK)]
      E1 --> E2 --> E3
      E3 --> E4 --> E5
      E4 --> E6
    end

    subgraph Application
      W1[User Auth (JWT)]
      W2[Query Cosmos DB & Smart Contract]
      W3[Render Dashboard & Scan UI]
      W1 --> W2 --> W3
    end
```

- Mote: Lightweight firmware reads tag, senses environment, hashes data, transmits.
- Edge & Cloud: Gateway aggregates, Azure IoT Hub ingests, Azure Functions store raw logs in Azure CosmosDB and push hashes on-chain. Data is also stored off-chain for fatster look ups and is verified using on-chain hash.
- Application: Stakeholders login or use JWT-protected QR codes to sign into dashboard which fetches combined on-chain/off-chain data for visualization, showing alerts. Clients submit product orders through dashboard and identify the requirements that the product must be shipped in.
