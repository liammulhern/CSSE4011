<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useProductStore } from '@/stores/product'
import { useForm } from 'vee-validate'
import { toTypedSchema } from '@vee-validate/zod'
import * as z from 'zod'

import { Button } from '@/components/ui/button'
import { FormField, FormItem, FormLabel, FormControl, FormDescription, FormMessage } from '@/components/ui/form'
import { Input } from '@/components/ui/input'
import { Dialog, DialogTrigger, DialogContent, DialogHeader, DialogTitle, DialogDescription, DialogFooter, DialogClose } from '@/components/ui/dialog'
import ProductStatusTable from '@/views/dashboard/pages/home/ProductStatusTable.vue'

// Vee-validate schema
const formSchema = toTypedSchema(
  z.object({
    product_key: z.string().min(1, 'Product key is required'),
    product_type: z.string().optional(),
    batch: z.string().min(1, 'Batch is required'),
    owner: z.string().optional(),
  })
)
const form = useForm({ validationSchema: formSchema })
const onSubmit = form.handleSubmit(async (values) => {
  const created = await store.createProduct({
    product_key: values.product_key,
    product_type: values.product_type ? Number(values.product_type) : null,
    batch: values.batch,
    owner: values.owner ? Number(values.owner) : null,
  })
  router.push({ name: 'product', params: { id: String(created.id) } })
})

// Table control
const router = useRouter()
const store = useProductStore()
const showForm = ref(false)
function toggleForm() {
  showForm.value = !showForm.value
}
</script>

<template>
  <Dialog v-model:open="showForm">
    <DialogTrigger asChild>
      <Button variant="outline">
        <Icon name="Plus" class="mr-2" />
        New Product
      </Button>
    </DialogTrigger>

    <DialogContent class="sm:max-w-lg">
      <DialogHeader>
        <DialogTitle>Create New Product</DialogTitle>
        <DialogDescription>Fill in the details to create a new product.
        </DialogDescription>
      </DialogHeader>

      <form @submit.prevent="onSubmit" class="space-y-4">
        <FormField name="product_key" v-slot="{ field }">
          <FormItem>
            <FormLabel>Product Key</FormLabel>
            <FormControl>
              <Input placeholder="Enter unique key" v-bind="field" />
            </FormControl>
            <FormDescription>Unique identifier for product.</FormDescription>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="product_type" v-slot="{ field }">
          <FormItem>
            <FormLabel>Product Type ID</FormLabel>
            <FormControl>
              <Input type="number" placeholder="Optional numeric type" v-bind="field" />
            </FormControl>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="batch" v-slot="{ field }">
          <FormItem>
            <FormLabel>Batch</FormLabel>
            <FormControl>
              <Input placeholder="Batch identifier" v-bind="field" />
            </FormControl>
            <FormDescription>Batch code for traceability.</FormDescription>
            <FormMessage />
          </FormItem>
        </FormField>

        <FormField name="owner" v-slot="{ field }">
          <FormItem>
            <FormLabel>Owner ID</FormLabel>
            <FormControl>
              <Input type="number" placeholder="Optional owner user ID" v-bind="field" />
            </FormControl>
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
