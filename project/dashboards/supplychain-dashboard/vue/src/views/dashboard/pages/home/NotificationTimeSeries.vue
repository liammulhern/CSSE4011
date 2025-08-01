<template>
  <div class="w-full h-full">
    <Line
      :data="chartData"
      :options="chartOptions"
      style="width: 100%; height: 100%;"
    />
  </div>
</template>

<script setup lang="ts">
import {
  ref,
  onMounted,
  onBeforeUnmount,
  computed,
  watch,
  defineProps
} from 'vue'
import http from '@/utils/http'
import {
  Chart as ChartJS,
  Title,
  Tooltip,
  Legend,
  LineElement,
  PointElement,
  LinearScale,
  TimeScale,
  CategoryScale,
  Filler
} from 'chart.js'
import 'chartjs-adapter-date-fns'
import { format } from 'date-fns'
import { Line } from 'vue-chartjs'

// Register chart.js components
ChartJS.register(
  Title,
  Tooltip,
  Legend,
  LineElement,
  PointElement,
  LinearScale,
  TimeScale,
  CategoryScale,
  Filler
)

interface LiveEvent {
  message_id: string
  event_type: string
  timestamp: string
  source: 'product' | 'tracker'
  ref_id: number | string | null
}

// Accept dateRange prop from parent
const props = defineProps<{
  dateRange: { start: Date | null; end: Date | null }
}>()

// State
const productEvents = ref<LiveEvent[]>([])
const trackerEvents = ref<LiveEvent[]>([])
const loading = ref(false)
const error = ref<string | null>(null)
let intervalId: ReturnType<typeof setInterval>

// Read Tailwind CSS HSL variables and parse into H, S, L
const rootStyles = getComputedStyle(document.documentElement)
const primaryHSLRaw = rootStyles.getPropertyValue('--primary').trim() // e.g. "21.1 72.2% 53.5%"
const secondaryHSLRaw = rootStyles.getPropertyValue('--secondary').trim()
const [h1, s1, l1] = primaryHSLRaw.split(/\s+/)
const [h2, s2, l2] = secondaryHSLRaw.split(/\s+/)
const primaryColor = `hsl(${h1}, ${s1}, ${l1})`
const secondaryColor = `hsl(${h2}, ${s2}, ${l2})`

// Fetch both endpoints every 10s
async function fetchEvents() {
  loading.value = true
  error.value = null
  try {
    const [pe, te] = await Promise.all([
      http.get<any[]>('/api/notifications/productevent'),
      //http.get<any[]>('/api/notifications/tracker')
    ])

    productEvents.value = pe.data.map(e => ({
      id: e.id,
      message: e.message,
      notication_type: e.notication_type,
      timestamp: e.timestamp,
      source: 'product',
      ref_id: e.productevent
    }))

    trackerEvents.value = te.data.map(e => ({
      id: e.id,
      message: e.message,
      notication_type: e.notication_type,
      timestamp: e.timestamp,
      source: 'tracker',
      ref_id: null 
    }))

    console.log(productEvents.value);
    console.log(trackerEvents.value);

  } catch (e: any) {
    error.value = e.message || String(e)
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  fetchEvents()
  intervalId = setInterval(fetchEvents, 10000)
})
onBeforeUnmount(() => clearInterval(intervalId))

// Filter events to the selected dateRange
function filterByRange(events: LiveEvent[]) {
  if (!props.dateRange.start || !props.dateRange.end) return events

  const startTs = props.dateRange.start.getTime()
  const endTs = props.dateRange.end.getTime()

  return events.filter(e => {
    const t = new Date(e.timestamp).getTime()
    return t >= startTs && t <= endTs
  })
}

// Aggregate into daily buckets
function aggregateByTime(data: LiveEvent[]) {
  const buckets: Record<string, number> = {}

  data.forEach(n => {
    const day = n.timestamp.slice(0, 10) // YYYY-MM-DD

    buckets[day] = (buckets[day] || 0) + 1
  })

  console.log(buckets)

  return Object.entries(buckets)
    .map(([day, count]) => ({ x: new Date(day), y: count }))
    .sort((a, b) => a.x.getTime() - b.x.getTime())
}

// Dynamic unit based on range length
const xUnit = computed<'day' | 'week' | 'month'>(() => {
  if (!props.dateRange.start || !props.dateRange.end) return 'day'

  const diffDays = (props.dateRange.end.getTime() - props.dateRange.start.getTime()) / (1000 * 60 * 60 * 24)

  if (diffDays <= 14) return 'day'
  if (diffDays <= 90) return 'week'

  return 'month'
})

// Build the two-series dataset
const chartData = computed(() => ({
  datasets: [
    {
      label: 'Product Events',
      data: aggregateByTime(filterByRange(productEvents.value)),
      borderColor: '#3B82F6',
      borderColor: primaryColor,
      tension: 0.4,
      fill: true,
      backgroundColor: ctx => {
        const g = ctx.chart.ctx.createLinearGradient(0, 0, 0, ctx.chart.height)
        g.addColorStop(0, `hsla(${h1}, ${s1}, ${l1}, 0.3)`)
        g.addColorStop(1, `hsla(${h1}, ${s1}, ${l1}, 0)`)
        return g
      },
      pointRadius: 0,
      pointHoverRadius: 4
    },
    {
      label: 'Tracker Events',
      data: aggregateByTime(filterByRange(trackerEvents.value)),
      borderColor: '#8B5CF6',
      borderColor: secondaryColor,
      tension: 0.4,
      fill: true,
      backgroundColor: ctx => {
        const g = ctx.chart.ctx.createLinearGradient(0, 0, 0, ctx.chart.height)
        g.addColorStop(0, `hsla(${h2}, ${s2}, ${l2}, 0.3)`)
        g.addColorStop(1, `hsla(${h2}, ${s2}, ${l2}, 0)`)
        return g
      },
      pointRadius: 0,
      pointHoverRadius: 4
    }
  ]
}))

// Chart.js options
const chartOptions = computed(() => ({
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: {
      position: 'right' as const,
      align: 'center' as const,
      labels: { usePointStyle: true, pointStyle: 'circle', font: { size: 14 }, boxWidth: 10 }
    },
    tooltip: {
      enabled: true,
      backgroundColor: '#1e293b',
      titleFont: { size: 14, weight: 'bold' },
      bodyFont: { size: 13 },
      callbacks: {
        title: items => format(items[0].parsed.x, 'PP'),
        label: item => `${item.dataset.label}: ${item.parsed.y} events`
      }
    }
  },
  scales: {
    x: {
      type: 'time' as const,
      time: {
        unit: xUnit.value,
        tooltipFormat: 'PP',
        displayFormats: { day: 'MMM d', week: 'MMM d', month: 'MMM yyyy' }
      },
      ticks: { autoSkip: true, maxTicksLimit: 12, maxRotation: 0, font: { size: 12 } },
      grid: { display: false }
    },
    y: {
      beginAtZero: true,
      ticks: { font: { size: 12 } },
      grid: { color: 'rgba(0,0,0,0.05)' }
    }
  }
}))

// Re-run when dateRange changes
watch(
  () => props.dateRange,
  () => {
    // triggers recompute of xUnit, chartData, chartOptions
  },
  { deep: true }
)
</script>

