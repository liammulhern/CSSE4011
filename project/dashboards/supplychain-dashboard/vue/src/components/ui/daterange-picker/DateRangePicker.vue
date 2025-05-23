<script setup lang="ts">
import { addDays, format, isValid } from 'date-fns'
import { Calendar as CalendarIcon } from 'lucide-vue-next'
import { computed } from 'vue'
import { cn } from '@/lib/utils'
import { Button } from '@/components/ui/button'
import { Calendar } from '@/components/ui/calendar'
import {
  Popover,
  PopoverContent,
  PopoverTrigger,
} from '@/components/ui/popover'

// Props and emits
const props = defineProps<{
  range: { start: Date | string | null; end: Date | string | null }
}>()
const emit = defineEmits<{
  (e: 'update:range', value: { start: Date | string | null; end: Date | string | null }): void
}>()

// Simple computed proxy: get/set object
const modelRange = computed<{
  start: Date | string | null
  end: Date | string | null
}>({
  get() {
    return props.range
  },
  set(val) {
    emit('update:range', val)
  }
})

// Computed label, handling string or Date inputs
const dateLabel = computed(() => {
  const { start, end } = props.range
  if (start != null) {
    const sDate = start instanceof Date ? start : new Date(start)
    if (isValid(sDate)) {
      if (end != null) {
        const eDate = end instanceof Date ? end : new Date(end)
        if (isValid(eDate)) {
          return `${format(sDate, 'LLL dd, y')} - ${format(eDate, 'LLL dd, y')}`
        }
      }
      return format(sDate, 'LLL dd, y')
    }
  }
  return 'Pick a date'
})
</script>

<template>
  <div :class="cn('grid gap-2', $attrs.class ?? '')">
    <Popover>
      <PopoverTrigger as-child>
        <Button variant="outline" class="w-[260px] justify-start text-left font-normal">
          <CalendarIcon class="mr-2 h-4 w-4" />
          <span>{{ dateLabel }}</span>
        </Button>
      </PopoverTrigger>
      <PopoverContent class="w-auto p-0" align="end">
        <!-- Bind calendar directly to computed proxy -->
        <Calendar v-model.range="modelRange" :columns="2" />
      </PopoverContent>
    </Popover>
  </div>
</template>
