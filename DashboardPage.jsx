import { useEffect, useState } from "react";
import Plot from "react-plotly.js";

// -----------------------------------------------------------------------------
// Dashboard de Batimentos Cardíacos (Heart Rate) - FIWARE + Plotly + React
// -----------------------------------------------------------------------------

export default function DashboardPage() {
  const [heartData, setHeartData] = useState([]);
  const [lastUpdate, setLastUpdate] = useState(null);

  // Altere este IP para o da sua VM no Azure
  const VM_IP = "20.164.0.231";

  // Função para buscar os dados históricos do STH-Comet
  const fetchHeartRate = async () => {
    try {
      const response = await fetch(
        `/api/sth/STH/v1/contextEntities/type/Player/id/urn:ngsi-ld:Player:001/attributes/heart_rate?lastN=20`,
        { headers: { "fiware-service": "smart", "fiware-servicepath": "/" } }
      );

      const data = await response.json();
      const values =
        data.contextResponses?.[0]?.contextElement?.attributes?.[0]?.values ||
        [];

      // Formata valores com horário legível e ordenado
      const formatted = values.map((item) => {
        const date = new Date(item.recvTime);
        return {
          time: date.toLocaleTimeString("pt-BR", {
            hour12: false,
            hour: "2-digit",
            minute: "2-digit",
            second: "2-digit",
          }),
          bpm: Number(item.attrValue),
        };
      });

      // Mantém apenas os dados mais recentes
      const recent = formatted.slice(-20);
      setHeartData(recent);
      setLastUpdate(new Date());
    } catch (error) {
      console.error("Erro ao buscar dados do STH-Comet:", error);
    }
  };

  // Atualiza dados a cada 10 segundos
  useEffect(() => {
    fetchHeartRate();
    const interval = setInterval(() => {
      fetchHeartRate();
    }, 10000);
    return () => clearInterval(interval);
  }, []);

  // Retorna o valor atual (último BPM)
  const currentBPM =
    heartData.length > 0 ? heartData[heartData.length - 1].bpm : "--";

  // Define o status com base no BPM atual
  const getStatus = (bpm) => {
    if (!bpm) return "Sem dados";
    if (bpm < 60) return "Repouso";
    if (bpm <= 100) return "Normal";
    if (bpm <= 140) return "Aquecimento";
    return "Alto Esforço";
  };

  const status = getStatus(currentBPM);

  // Define cor do texto do status dinamicamente
  const getStatusColor = (status) => {
    switch (status) {
      case "Normal":
        return "text-green-400";
      case "Aquecimento":
        return "text-yellow-400";
      case "Alto Esforço":
        return "text-red-500";
      case "Repouso":
        return "text-blue-400";
      default:
        return "text-gray-400";
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-50 to-blue-50 text-white flex flex-col items-center py-10 px-6">
      <h1 className="text-3xl font-bold mb-2 text-primary">
        Dashboard - Heart Rate Player
      </h1>
      <p className="text-sm text-gray-400 mb-6">
        Frequência cardíaca monitorada em tempo real
      </p>

      <div className="w-full max-w-3xl bg-primary rounded-2xl shadow-lg p-6 mb-6">
        <h2 className="text-xl font-semibold mb-2">
          Batimentos Cardíacos (BPM)
        </h2>
        <Plot
          data={[
            {
              x: heartData.map((item) => item.time),
              y: heartData.map((item) => item.bpm),
              type: "scatter",
              mode: "lines+markers",
              marker: { color: "#ef4444", size: 6 },
              line: { color: "#f87171", width: 3, shape: "spline" },
              name: "Batimentos",
            },
          ]}
          layout={{
            autosize: true,
            paper_bgcolor: "#0f172a",
            plot_bgcolor: "#0f172a",
            font: { color: "#f1f5f9" },
            xaxis: {
              title: "Horário da Medição",
              tickangle: -45, // Inclina as horas
              tickmode: "array",
              tickvals: heartData.map((item, index) =>
                index % 2 === 0 ? item.time : ""
              ), // mostra só a cada 2 medições
              showgrid: false,
            },
            yaxis: {
              title: "Batimentos por Minuto (BPM)",
              range: [40, 200],
              gridcolor: "#1e293b",
            },
            margin: { t: 40, r: 20, l: 50, b: 90 },
          }}
          style={{ width: "100%", height: "400px" }}
          useResizeHandler={true}
        />
      </div>

      <div className="w-full max-w-3xl flex flex-col items-center gap-3 bg-primary rounded-2xl shadow-lg p-6">
        <h2 className="text-lg font-semibold text-gray-300">Status Atual</h2>
        <p className={`text-2xl font-bold ${getStatusColor(status)}`}>
          {status}
        </p>

        <p className="text-3xl font-bold text-blue-400">
          {currentBPM !== "--" ? `${currentBPM} BPM` : "--"}
        </p>

        {lastUpdate && (
          <p className="text-sm text-gray-400">
            Última atualização:{" "}
            {lastUpdate.toLocaleTimeString("pt-BR", { hour12: false })}
          </p>
        )}
      </div>
    </div>
  );
}
