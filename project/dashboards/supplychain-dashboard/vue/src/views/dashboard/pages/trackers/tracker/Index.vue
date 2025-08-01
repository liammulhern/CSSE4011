<script setup lang="ts">
import { h, onMounted, computed, defineProps } from 'vue'
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Button } from '@/components/ui/button'
import { useTrackerEventStore } from '@/stores/trackerevents'
import {
  Chart as ChartJS,
  Title,
  Tooltip,
  Legend,
  LineElement,
  PointElement,
  CategoryScale,
  LinearScale,
  TimeScale,
  Filler,
} from 'chart.js'
import { Line } from 'vue-chartjs'
import 'chartjs-adapter-date-fns'

// Register Chart.js components
ChartJS.register(
  Title,
  Tooltip,
  Legend,
  LineElement,
  PointElement,
  CategoryScale,
  LinearScale,
  TimeScale,
  Filler
)

// Props & store
const props = defineProps<{ id: number }>()
const store = useTrackerEventStore()

onMounted(() => {
  store.fetchEventsByTracker(props.id)
})

// Helper types & functions
type EventRow = { message_id: string; timestamp: string; [key: string]: string | number }
const flattenPayload = (payload: Record<string, any>): Record<string, string | number> => {
  const flat: Record<string, string | number> = {}
  Object.entries(payload).forEach(([k, v]) => {
    if (v && typeof v === 'object') {
      Object.entries(v).forEach(([subk, subv]) => {
        flat[`${k}_${subk}`] = subv
      })
    } else {
      flat[k] = v as string | number
    }
  })
  return flat
}

// Color palette for series
const COLORS = [
  '#3B82F6', // blue
  '#EF4444', // red
  '#10B981', // green
  '#F59E0B', // amber
  '#8B5CF6', // violet
  '#EC4899', // pink
]

// Table data
const data = computed<EventRow[]>(() =>
  store.events.map(e => ({
    message_id: e.message_id,
    timestamp: new Date(e.timestamp).toLocaleString(),
    ...flattenPayload(e.payload),
  }))
)

const tableColumns = computed<ColumnDef<EventRow>[]>(() => {
  if (!data.value.length) return []
  return Object.keys(data.value[0]).map(key => ({
    accessorKey: key,
    header: ({ column }) => h(DataTableHeader, { column, title: key.replace(/_/g, ' ') }),
    cell: ({ getValue }) => String(getValue()),
  }))
})

// Group numeric fields by prefix
const numericKeys = computed(() =>
  data.value.length
    ? Object.keys(data.value[0]).filter(k => k !== 'timestamp' && !isNaN(Number(data.value[0][k])))
    : []
)

const unitGroups = computed<Record<string, string[]>>(() => {
  const groups: Record<string, string[]> = {}
  numericKeys.value.forEach(key => {
    const grp = key.split('_')[0]
    ;(groups[grp] ||= []).push(key)
  })
  return groups
})

// Build per-group chartData with unique colours & gradients
const chartDataByGroup = computed(() =>
  Object.fromEntries(
    Object.entries(unitGroups.value).map(([group, keys]) => {
      const labels = data.value.map(r => new Date(r.timestamp))
      const datasets = keys.map((key, idx) => {
        const base = COLORS[idx % COLORS.length]
        return {
          label: key,
          data: data.value.map(r => Number(r[key])),
          borderColor: base,
          tension: 0.4,
          fill: true,
          backgroundColor: (ctx: any) => {
            const g = ctx.chart.ctx.createLinearGradient(0, 0, 0, ctx.chart.height)
            g.addColorStop(0, base + '33') // 20% alpha
            g.addColorStop(1, base + '00') // 0% alpha
            return g
          },
          pointRadius: 0,
          pointHoverRadius: 4,
        }
      })
      return [group, { labels, datasets }]
    })
  )
)

// Chart options (legend bottom for this view)
const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: {
      position: 'bottom' as const,
      align: 'center' as const,
      labels: {
        usePointStyle: true,
        pointStyle: 'circle',
        font: { size: 14 },
        boxWidth: 10,
      },
    },
    tooltip: {
      enabled: true,
      callbacks: {
        title: items => items[0].parsed.x.toLocaleDateString(),
        label: item => `${item.dataset.label}: ${item.parsed.y}`,
      },
    },
  },
  scales: {
    x: {
      type: 'time' as const,
      time: { tooltipFormat: 'PP', displayFormats: { hour: 'MMM d h a' } },
      ticks: { autoSkip: true, maxTicksLimit: 12, font: { size: 12 } },
      grid: { display: false },
    },
    y: {
      beginAtZero: true,
      ticks: { font: { size: 12 } },
      grid: { color: 'rgba(0,0,0,0.05)' },
    },
  },
}
</script>

<template>
  <div>
    <page-header title="Tracker Events" />

    <div class="mb-4 flex justify-between items-center">
      <h2 class="text-lg font-medium">Events for Tracker {{ props.id }}</h2>
      <Button :disabled="store.loading" @click="store.fetchEventsByTracker(props.id)">
        <Icon name="RefreshCw" class="pr-1" /> Refresh
      </Button>
    </div>

    <div v-if="store.loading" class="py-4 text-center">Loading events…</div>
    <div v-else-if="store.error" class="py-4 text-red-500">Error: {{ store.error }}</div>

    <div v-else-if="data.length">
      <DataTable :columns="tableColumns" :data="data" />

      <h2 class="text-lg font-medium my-4">Time Series</h2>
      <div
        v-for="(chartData, group) in chartDataByGroup"
        :key="group"
        class="mb-6 w-full h-64"
      >
        <h3 class="text-sm font-medium mb-2">{{ group.replace(/_/g, ' ') }}</h3>
        <div class="w-full h-full">
          <Line :data="chartData" :options="chartOptions" style="width: 100%; height: 100%;" />
        </div>
      </div>
    </div>

    <div v-else class="py-4 text-center">No events found.</div>
  </div>
</template>

