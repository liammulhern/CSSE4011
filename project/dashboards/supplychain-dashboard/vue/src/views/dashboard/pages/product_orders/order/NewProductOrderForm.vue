<script setup lang="ts">
import { ref, computed, h } from 'vue'
import { useRouter } from 'vue-router'
import { useProductOrderStore } from '@/stores/productorder'
import { useForm } from 'vee-validate'
import { toTypedSchema } from '@vee-validate/zod'
import { CalendarDate, DateFormatter, getLocalTimeZone, today, parseDate } from '@internationalized/date'
import * as z from 'zod'

import { Button } from '@/components/ui/button'
import { Dialog, DialogTrigger, DialogContent, DialogHeader, DialogTitle, DialogDescription, DialogFooter, DialogClose } from '@/components/ui/dialog'
import { FormField, FormItem, FormLabel, FormControl, FormDescription, FormMessage } from '@/components/ui/form'
import { Input } from '@/components/ui/input'
import { Popover, PopoverTrigger, PopoverContent } from '@/components/ui/popover'
import { Calendar } from '@/components/ui/calendar'
import { CalendarIcon } from 'lucide-vue-next'

const router = useRouter()
const store = useProductOrderStore()

// Dialog open state
const showForm = ref(false)

// Date formatting
const df = new DateFormatter('en-US', { dateStyle: 'medium' })

// Validation schema
const formSchema = toTypedSchema(
  z.object({
    order_number: z.string().min(1, 'Order number is required'),
    supplier_id: z.preprocess((val) => Number(val), z.number().positive('Supplier ID must be positive')),
    receiver_id: z.preprocess((val) => Number(val), z.number().positive('Receiver ID must be positive')),
    order_timestamp: z.string().refine((v) => !isNaN(Date.parse(v)), 'Invalid date'),
  })
)

// Vee-validate form
const { handleSubmit, setFieldValue, values } = useForm({ validationSchema: formSchema })

// Computed calendar value
const dateValue = computed({
  get: () => (values.order_timestamp ? parseDate(values.order_timestamp) : undefined),
  set: (val: CalendarDate | undefined) => {
    if (val) setFieldValue('order_timestamp', val.toString())
  }
})

// Submit handler
const onSubmit = handleSubmit(async (vals) => {
  const created = await store.createOrder({
    order_number: vals.order_number,
    supplier: Number(vals.supplier_id),
    receiver: Number(vals.receiver_id),
    order_timestamp: vals.order_timestamp,
  })
  showForm.value = false
  router.push({ name: 'product_order_id', params: { id: String(created.id) } })
})

function toggleForm() {
  showForm.value = !showForm.value
}
</script>

<template>
  <Dialog v-model:open="showForm">
    <DialogTrigger asChild>
      <Button variant="outline">
        <Icon name="Plus" class="mr-2" />
        New Product Order
      </Button>
    </DialogTrigger>

    <DialogContent class="sm:max-w-lg">
      <DialogHeader>
        <DialogTitle>Create Product Order</DialogTitle>
        <DialogDescription>Enter details for the new product order.</DialogDescription>
      </DialogHeader>

      <form @submit.prevent="onSubmit" class="space-y-4 mt-4">
        <FormField name="order_number" v-slot="{ field }">
          <FormItem>
            <FormLabel>Order Number</FormLabel>
            <FormControl>
              <Input placeholder="e.g. PO-1001" v-bind="field" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="supplier_id" v-slot="{ field }">
          <FormItem>
            <FormLabel>Supplier ID</FormLabel>
            <FormControl>
              <Input type="number" placeholder="Supplier user ID" v-bind="field" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="receiver_id" v-slot="{ field }">
          <FormItem>
            <FormLabel>Receiver ID</FormLabel>
            <FormControl>
              <Input type="number" placeholder="Receiver user ID" v-bind="field" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <!-- Calendar date picker -->
        <FormField name="order_timestamp">
          <FormItem class="flex flex-col">
            <FormLabel>Order Date</FormLabel>
            <Popover>
              <PopoverTrigger asChild>
                <FormControl>
                  <Button variant="outline"
                    :class="['w-[240px] ps-3 text-start font-normal', !dateValue && 'text-muted-foreground']">
                    <span>{{ dateValue ? df.format(dateValue) : 'Pick a date' }}</span>
                    <CalendarIcon class="ms-auto h-4 w-4 opacity-50" />
                  </Button>
                  <!-- hidden input to register field -->
                  <input type="hidden" v-model="values.order_timestamp" />
                </FormControl>
              </PopoverTrigger>
              <PopoverContent class="w-auto p-0">
                <Calendar v-model="dateValue" calendar-label="Order date" initial-focus
                  :min-value="new CalendarDate(2000, 1, 1)" :max-value="today(getLocalTimeZone())" />
              </PopoverContent>
            </Popover>
            <FormDescription>Select the date of the order.</FormDescription>
            <FormMessage />
          </FormItem>
        </FormField>

        <div class="flex justify-end space-x-2">
          <DialogClose asChild>
            <Button variant="secondary" type="button" @click="toggleForm">
              Cancel
            </Button>
          </DialogClose>
          <Button type="submit">
            Create
          </Button>
        </div>
      </form>
    </DialogContent>
  </Dialog>
</template>
