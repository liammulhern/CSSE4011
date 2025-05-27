<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useForm } from 'vee-validate'
import { toTypedSchema } from '@vee-validate/zod'
import * as z from 'zod'
import { useProductTypeStore } from '@/stores/producttype'
import { Button } from '@/components/ui/button'
import { FormField, FormItem, FormLabel, FormControl, FormMessage } from '@/components/ui/form'
import { Input } from '@/components/ui/input'
import { Dialog, DialogTrigger, DialogContent, DialogHeader, DialogTitle, DialogDescription, DialogFooter, DialogClose } from '@/components/ui/dialog'

const router = useRouter()
const store = useProductTypeStore()

const showForm = ref(false)

// Validation schema
const schema = toTypedSchema(
    z.object({
        product_number: z.string().min(1, 'SKU is required'),
        name: z.string().min(1, 'Name is required'),
        description: z.string().optional(),
    })
)

const { handleSubmit, values } = useForm({ validationSchema: schema })

const onSubmit = handleSubmit(async (vals) => {
    const created = await store.createProductType({
        product_number: vals.product_number,
        name: vals.name,
        description: vals.description || null,
        owner: null,
    })
    showForm.value = false
    router.push({ name: 'product_type_detail', params: { id: String(created.id) } })
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
                New Product Type
            </Button>
        </DialogTrigger>

        <DialogContent class="sm:max-w-lg">
            <DialogHeader>
                <DialogTitle>Create New Product Type</DialogTitle>
                <DialogDescription>Fill in the details to create a new product type.
                </DialogDescription>
            </DialogHeader>

            <form @submit.prevent="onSubmit" class="space-y-4 mt-4">
                <FormField name="product_number" v-slot="{ field }">
                    <FormItem>
                        <FormLabel>Product Number (SKU)</FormLabel>
                        <FormControl><Input v-bind="field" /></FormControl>
                        <FormMessage />
                    </FormItem>
                </FormField>

                <FormField name="name" v-slot="{ field }">
                    <FormItem>
                        <FormLabel>Name</FormLabel>
                        <FormControl><Input v-bind="field" /></FormControl>
                        <FormMessage />
                    </FormItem>
                </FormField>

                <FormField name="description" v-slot="{ field }">
                    <FormItem class='flex flex-col'>
                        <FormLabel>Description</FormLabel>
                        <FormControl>
                            <Textarea class="resize-none border rounded-md" rows="4" v-bind="field" />
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
